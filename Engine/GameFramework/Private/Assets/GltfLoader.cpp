#define CGLTF_IMPLEMENTATION
#include "PCH.h"
#include "GameFramework/Public/Assets/GltfLoader.h"

#include <cgltf.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
#include <span>

using namespace DirectX;

GltfLoader::CgltfGuard::~CgltfGuard()
{
	cgltf_free(ptr);
}

bool GltfLoader::ValidateInputPath(const std::filesystem::path& filePath, LoadResult& result)
{
	if (std::filesystem::exists(filePath))
	{
		return true;
	}

	result.errorMessage = std::format("GltfLoader: File not found: {}", filePath.string());
	LOG_ERROR(result.errorMessage);
	return false;
}

bool GltfLoader::ParseGltfFile(cgltf_options& options, const std::string& pathStr, cgltf_data*& outData, LoadResult& result)
{
	cgltf_result parseResult = cgltf_parse_file(&options, pathStr.c_str(), &outData);
	if (parseResult == cgltf_result_success)
	{
		return true;
	}

	result.errorMessage = std::format("GltfLoader: Failed to parse '{}' (cgltf error {})", pathStr, static_cast<int>(parseResult));
	LOG_ERROR(result.errorMessage);
	return false;
}

bool GltfLoader::LoadGltfBuffers(cgltf_options& options, cgltf_data* data, const std::string& pathStr, LoadResult& result)
{
	cgltf_result bufferResult = cgltf_load_buffers(&options, data, pathStr.c_str());
	if (bufferResult == cgltf_result_success)
	{
		return true;
	}

	result.errorMessage =
	    std::format("GltfLoader: Failed to load buffers for '{}' (cgltf error {})", pathStr, static_cast<int>(bufferResult));
	LOG_ERROR(result.errorMessage);
	return false;
}


void GltfLoader::ValidateGltf(cgltf_data* data, const std::string& pathStr)
{
	cgltf_result validateResult = cgltf_validate(data);
	if (validateResult != cgltf_result_success)
	{
		LOG_WARNING(std::format("GltfLoader: Validation warnings for '{}' (cgltf error {})", pathStr, static_cast<int>(validateResult)));
	}
}

std::size_t GltfLoader::CountTotalPrimitives(const cgltf_data* data)
{
	std::size_t totalPrimitives = 0;
	for (cgltf_size n = 0; n < data->nodes_count; ++n)
	{
		const cgltf_node& node = data->nodes[n];
		if (node.mesh)
		{
			totalPrimitives += node.mesh->primitives_count;
		}
	}
	return totalPrimitives;
}

void GltfLoader::ExtractMeshesFromNodes(const cgltf_data* data, LoadResult& result)
{
	for (cgltf_size n = 0; n < data->nodes_count; ++n)
	{
		const cgltf_node& node = data->nodes[n];
		if (!node.mesh)
		{
			continue;
		}

		const XMMATRIX worldTransform = ComputeNodeWorldTransform(&node);

		for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p)
		{
			const cgltf_primitive& primitive = node.mesh->primitives[p];

			if (primitive.type != cgltf_primitive_type_triangles)
			{
				continue;
			}

			MeshData meshData = ExtractPrimitive(primitive);
			if (!meshData.IsValid())
			{
				continue;
			}

			result.materialIndices.push_back(ResolveMaterialIndex(primitive, data));
			result.transforms.emplace_back(worldTransform);
			result.meshes.push_back(std::move(meshData));
		}
	}
}

template <typename T> T GltfLoader::ReadAccessorElement(const cgltf_accessor* accessor, std::size_t index)
{
	T result{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&result), sizeof(T) / sizeof(float));
	}
	return result;
}

const cgltf_accessor* GltfLoader::FindAttribute(const cgltf_primitive& primitive, int type)
{
	for (cgltf_size i = 0; i < primitive.attributes_count; ++i)
	{
		if (primitive.attributes[i].type == static_cast<cgltf_attribute_type>(type))
		{
			return primitive.attributes[i].data;
		}
	}
	return nullptr;
}

void GltfLoader::ReadIndices(const cgltf_accessor* accessor, std::vector<uint32_t>& outIndices)
{
	if (!accessor)
		return;

	outIndices.resize(accessor->count);
	for (cgltf_size i = 0; i < accessor->count; ++i)
	{
		outIndices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(accessor, i));
	}
}

XMMATRIX GltfLoader::ComputeNodeWorldTransform(const cgltf_node* node)
{
	XMMATRIX world = XMMatrixIdentity();

	const cgltf_node* chain[64];
	int depth = 0;

	for (const cgltf_node* n = node; n != nullptr && depth < 64; n = n->parent)
	{
		chain[depth++] = n;
	}

	for (int i = depth - 1; i >= 0; --i)
	{
		float localMatrix[16];
		cgltf_node_transform_local(chain[i], localMatrix);

		XMMATRIX local = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(localMatrix));
		world = XMMatrixMultiply(world, local);
	}

	return world;
}

std::filesystem::path GltfLoader::ResolveImagePath(const cgltf_image* image, const std::filesystem::path& gltfDirectory)
{
	if (!image || !image->uri)
		return {};

	return gltfDirectory / image->uri;
}

void GltfLoader::ExtractMaterials(
    const cgltf_data* data,
    const std::filesystem::path& gltfDirectory,
	std::vector<MaterialDesc>& outMaterials)
{
	outMaterials.reserve(data->materials_count);

	for (cgltf_size i = 0; i < data->materials_count; ++i)
	{
		const cgltf_material& mat = data->materials[i];

		MaterialDesc desc;
		desc.name = mat.name ? mat.name : std::format("Material_{}", i);
		desc.emissiveColor = XMFLOAT3(mat.emissive_factor[0], mat.emissive_factor[1], mat.emissive_factor[2]);
		desc.alphaCutoff = mat.alpha_cutoff;

		switch (mat.alpha_mode)
		{
			case cgltf_alpha_mode_mask:
				desc.alphaMode = AlphaMode::Mask;
				break;
			case cgltf_alpha_mode_blend:
				desc.alphaMode = AlphaMode::Blend;
				break;
			case cgltf_alpha_mode_opaque:
			default:
				desc.alphaMode = AlphaMode::Opaque;
				break;
		}

		if (mat.has_pbr_metallic_roughness)
		{
			const auto& pbr = mat.pbr_metallic_roughness;

			desc.baseColor =
			    XMFLOAT4(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]);

			desc.metallic = pbr.metallic_factor;
			desc.roughness = pbr.roughness_factor;

			if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image)
			{
				auto path = ResolveImagePath(pbr.base_color_texture.texture->image, gltfDirectory);
				if (!path.empty())
				{
					desc.albedoTexture = path;
				}
			}

			if (pbr.metallic_roughness_texture.texture && pbr.metallic_roughness_texture.texture->image)
			{
				auto path = ResolveImagePath(pbr.metallic_roughness_texture.texture->image, gltfDirectory);
				if (!path.empty())
				{
					desc.metallicRoughnessTexture = path;
				}
			}
		}

		if (mat.normal_texture.texture && mat.normal_texture.texture->image)
		{
			auto path = ResolveImagePath(mat.normal_texture.texture->image, gltfDirectory);
			if (!path.empty())
			{
				desc.normalTexture = path;
			}
		}

		if (mat.occlusion_texture.texture && mat.occlusion_texture.texture->image)
		{
			auto path = ResolveImagePath(mat.occlusion_texture.texture->image, gltfDirectory);
			if (!path.empty())
			{
				desc.occlusionTexture = path;
			}
		}

		if (mat.emissive_texture.texture && mat.emissive_texture.texture->image)
		{
			auto path = ResolveImagePath(mat.emissive_texture.texture->image, gltfDirectory);
			if (!path.empty())
			{
				desc.emissiveTexture = path;
			}
		}

		outMaterials.push_back(std::move(desc));
	}
}

std::uint32_t GltfLoader::ResolveMaterialIndex(const cgltf_primitive& primitive, const cgltf_data* data)
{
	if (!primitive.material)
		return 0;

	auto index = static_cast<std::uint32_t>(primitive.material - data->materials);
	return index;
}

MeshData GltfLoader::ExtractPrimitive(const cgltf_primitive& primitive)
{
	const cgltf_accessor* positions = FindAttribute(primitive, cgltf_attribute_type_position);
	const cgltf_accessor* normals = FindAttribute(primitive, cgltf_attribute_type_normal);
	const cgltf_accessor* texcoords = FindAttribute(primitive, cgltf_attribute_type_texcoord);
	const cgltf_accessor* tangents = FindAttribute(primitive, cgltf_attribute_type_tangent);

	if (!positions)
		return {};

	const auto vertexCount = static_cast<uint32_t>(positions->count);

	MeshData meshData;
	meshData.Reserve(vertexCount, primitive.indices ? static_cast<uint32_t>(primitive.indices->count) : 0);

	meshData.vertices.resize(vertexCount);

	for (uint32_t v = 0; v < vertexCount; ++v)
	{
		VertexData& vertex = meshData.vertices[v];

		vertex.position = ReadAccessorElement<XMFLOAT3>(positions, v);

		if (normals)
		{
			vertex.normal = ReadAccessorElement<XMFLOAT3>(normals, v);
		}

		if (texcoords)
		{
			vertex.uv = ReadAccessorElement<XMFLOAT2>(texcoords, v);
		}

		if (tangents)
		{
			vertex.tangent = ReadAccessorElement<XMFLOAT4>(tangents, v);
		}

		vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
	}

	ReadIndices(primitive.indices, meshData.indices);

	return meshData;
}

GltfLoader::LoadResult GltfLoader::Load(const std::filesystem::path& filePath)
{
	LoadResult result;

	if (!ValidateInputPath(filePath, result))
	{
		return result;
	}

	const std::string pathStr = filePath.string();
	const std::filesystem::path gltfDirectory = filePath.parent_path();

	cgltf_options options{};
	cgltf_data* data = nullptr;

	if (!ParseGltfFile(options, pathStr, data, result))
	{
		return result;
	}

	CgltfGuard guard{data};

	if (!LoadGltfBuffers(options, data, pathStr, result))
	{
		return result;
	}

	ValidateGltf(data, pathStr);

	ExtractMaterials(data, gltfDirectory, result.materials);

	const std::size_t totalPrimitives = CountTotalPrimitives(data);

	result.meshes.reserve(totalPrimitives);
	result.transforms.reserve(totalPrimitives);
	result.materialIndices.reserve(totalPrimitives);

	ExtractMeshesFromNodes(data, result);

	result.bSuccess = true;

	LOG_INFO(
	    std::format(
	        "GltfLoader: Loaded '{}' — {} meshes, {} materials",
	        filePath.filename().string(),
	        result.meshes.size(),
	        result.materials.size()));

	return result;
}
