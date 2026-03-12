#define CGLTF_IMPLEMENTATION
#include "PCH.h"
#include "GameFramework/Public/Assets/GltfLoader.h"

#include <cgltf.h>

#include <algorithm>
#include <cstring>
#include <format>
#include <span>

using namespace DirectX;

// =============================================================================
// Internal Helpers
// =============================================================================

namespace GltfLoaderInternal
{
	XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	MeshData ExtractPrimitive(const cgltf_primitive& primitive);
	std::uint32_t ResolveMaterialIndex(const cgltf_primitive& primitive, const cgltf_data* data);

	struct CgltfGuard
	{
		cgltf_data* ptr = nullptr;
		~CgltfGuard() { cgltf_free(ptr); }
	};

	bool ValidateInputPath(const std::filesystem::path& filePath, GltfLoader::LoadResult& result)
	{
		if (std::filesystem::exists(filePath))
		{
			return true;
		}

		result.errorMessage = std::format("GltfLoader: File not found: {}", filePath.string());
		LOG_ERROR(result.errorMessage);
		return false;
	}

	bool ParseGltfFile(
	    cgltf_options& options,
	    const std::string& pathStr,
	    cgltf_data*& outData,
	    GltfLoader::LoadResult& result)
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

	bool LoadGltfBuffers(
	    cgltf_options& options,
	    cgltf_data* data,
	    const std::string& pathStr,
	    GltfLoader::LoadResult& result)
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

	void ValidateGltf(cgltf_data* data, const std::string& pathStr)
	{
		cgltf_result validateResult = cgltf_validate(data);
		if (validateResult != cgltf_result_success)
		{
			LOG_WARNING(std::format("GltfLoader: Validation warnings for '{}' (cgltf error {})", pathStr, static_cast<int>(validateResult)));
		}
	}

	void EnsureDefaultMaterial(GltfLoader::LoadResult& result)
	{
		if (!result.materials.empty())
		{
			return;
		}

		MaterialDesc defaultMat;
		defaultMat.name = "Default";
		result.materials.push_back(std::move(defaultMat));
	}

	std::size_t CountTotalPrimitives(const cgltf_data* data)
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

	void ExtractMeshesFromNodes(const cgltf_data* data, GltfLoader::LoadResult& result)
	{
		for (cgltf_size n = 0; n < data->nodes_count; ++n)
		{
			const cgltf_node& node = data->nodes[n];
			if (!node.mesh)
			{
				continue;
			}

			const XMMATRIX worldTransform = ComputeNodeWorldTransform(&node);

			XMFLOAT4X4 worldMatrix;
			XMStoreFloat4x4(&worldMatrix, worldTransform);

			for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p)
			{
				const cgltf_primitive& primitive = node.mesh->primitives[p];

				// Only triangle geometry is supported
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
				result.transforms.push_back(worldMatrix);
				result.meshes.push_back(std::move(meshData));
			}
		}
	}

	// Typed read from a cgltf accessor at a given element index.
	// Returns a zeroed value if the accessor is null or index is out of range.
	template <typename T> T ReadAccessorElement(const cgltf_accessor* accessor, cgltf_size index)
	{
		T result{};
		if (accessor && index < accessor->count)
		{
			cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&result), sizeof(T) / sizeof(float));
		}
		return result;
	}

	// Finds the accessor for a named attribute in a primitive (POSITION, NORMAL, etc.)
	const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, cgltf_attribute_type type)
	{
		for (cgltf_size i = 0; i < primitive.attributes_count; ++i)
		{
			if (primitive.attributes[i].type == type)
			{
				return primitive.attributes[i].data;
			}
		}
		return nullptr;
	}

	// Reads all indices from a primitive's index accessor into a uint32 vector.
	void ReadIndices(const cgltf_accessor* accessor, std::vector<uint32_t>& outIndices)
	{
		if (!accessor)
			return;

		outIndices.resize(accessor->count);
		for (cgltf_size i = 0; i < accessor->count; ++i)
		{
			outIndices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(accessor, i));
		}
	}

	// Computes the world transform for a node by walking up the parent chain.
	// cgltf provides local transforms; we accumulate to world space.
	XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node)
	{
		XMMATRIX world = XMMatrixIdentity();

		// Walk up the hierarchy, accumulating local transforms
		// We collect them first, then multiply in root-to-leaf order
		const cgltf_node* chain[64];
		int depth = 0;

		for (const cgltf_node* n = node; n != nullptr && depth < 64; n = n->parent)
		{
			chain[depth++] = n;
		}

		// Multiply root-to-leaf (reverse of collection order)
		for (int i = depth - 1; i >= 0; --i)
		{
			float localMatrix[16];
			cgltf_node_transform_local(chain[i], localMatrix);

			// cgltf stores matrices in column-major order (OpenGL convention).
			// DirectXMath uses row-major storage, so we transpose.
			XMMATRIX local = XMMatrixTranspose(XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(localMatrix)));
			world = XMMatrixMultiply(world, local);
		}

		return world;
	}

	// Resolves the file path for a cgltf image relative to the glTF file directory.
	std::filesystem::path ResolveImagePath(const cgltf_image* image, const std::filesystem::path& gltfDirectory)
	{
		if (!image || !image->uri)
			return {};

		// URI may be percent-encoded; cgltf_decode_uri handles this in-place,
		// but we don't want to mutate the data — just use the raw URI for now.
		// Most glTF exporters produce simple relative paths without encoding.
		return gltfDirectory / image->uri;
	}

	// Extracts material descriptions from a parsed glTF scene.
	void ExtractMaterials(
	    const cgltf_data* data,
	    const std::filesystem::path& gltfDirectory,
	    std::vector<MaterialDesc>& outMaterials,
	    std::vector<std::string>& outTexturePaths)
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

			auto addUniqueTexturePath = [&outTexturePaths](const std::filesystem::path& path)
			{
				auto pathStr = path.string();
				if (std::ranges::find(outTexturePaths, pathStr) == outTexturePaths.end())
				{
					outTexturePaths.push_back(std::move(pathStr));
				}
			};

			if (mat.has_pbr_metallic_roughness)
			{
				const auto& pbr = mat.pbr_metallic_roughness;

				desc.baseColor =
				    XMFLOAT4(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]);

				desc.metallic = pbr.metallic_factor;
				desc.roughness = pbr.roughness_factor;

				// Albedo texture
				if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image)
				{
					auto path = ResolveImagePath(pbr.base_color_texture.texture->image, gltfDirectory);
					if (!path.empty())
					{
						desc.albedoTexture = path;
						addUniqueTexturePath(path);
					}
				}

				// Metallic-roughness texture
				if (pbr.metallic_roughness_texture.texture && pbr.metallic_roughness_texture.texture->image)
				{
					auto path = ResolveImagePath(pbr.metallic_roughness_texture.texture->image, gltfDirectory);
					if (!path.empty())
					{
						desc.metallicRoughnessTexture = path;
						addUniqueTexturePath(path);
					}
				}
			}

			// Normal map
			if (mat.normal_texture.texture && mat.normal_texture.texture->image)
			{
				auto path = ResolveImagePath(mat.normal_texture.texture->image, gltfDirectory);
				if (!path.empty())
				{
					desc.normalTexture = path;
					addUniqueTexturePath(path);
				}
			}

			// Occlusion texture
			if (mat.occlusion_texture.texture && mat.occlusion_texture.texture->image)
			{
				auto path = ResolveImagePath(mat.occlusion_texture.texture->image, gltfDirectory);
				if (!path.empty())
				{
					desc.occlusionTexture = path;
					addUniqueTexturePath(path);
				}
			}

			// Emissive texture
			if (mat.emissive_texture.texture && mat.emissive_texture.texture->image)
			{
				auto path = ResolveImagePath(mat.emissive_texture.texture->image, gltfDirectory);
				if (!path.empty())
				{
					desc.emissiveTexture = path;
					addUniqueTexturePath(path);
				}
			}

			outMaterials.push_back(std::move(desc));
		}
	}

	// Resolves the material index for a primitive. Returns 0 (default) if unassigned.
	std::uint32_t ResolveMaterialIndex(const cgltf_primitive& primitive, const cgltf_data* data)
	{
		if (!primitive.material)
			return 0;

		// cgltf stores materials in a flat array; pointer arithmetic gives the index
		auto index = static_cast<std::uint32_t>(primitive.material - data->materials);
		return index;
	}

	// Extracts vertex and index data for a single glTF primitive.
	MeshData ExtractPrimitive(const cgltf_primitive& primitive)
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

		// Build vertex array
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

		// Read index buffer
		ReadIndices(primitive.indices, meshData.indices);

		return meshData;
	}

}  // namespace GltfLoaderInternal

// =============================================================================
// GltfLoader::Load
// =============================================================================

GltfLoader::LoadResult GltfLoader::Load(const std::filesystem::path& filePath)
{
	LoadResult result;

	// -------------------------------------------------------------------------
	// Validate input
	// -------------------------------------------------------------------------

	if (!GltfLoaderInternal::ValidateInputPath(filePath, result))
	{
		return result;
	}

	const std::string pathStr = filePath.string();
	const std::filesystem::path gltfDirectory = filePath.parent_path();

	// -------------------------------------------------------------------------
	// Parse glTF
	// -------------------------------------------------------------------------

	cgltf_options options{};
	cgltf_data* data = nullptr;

	if (!GltfLoaderInternal::ParseGltfFile(options, pathStr, data, result))
	{
		return result;
	}

	// RAII cleanup — cgltf_free must be called regardless of early exits
	GltfLoaderInternal::CgltfGuard guard{data};

	// -------------------------------------------------------------------------
	// Load buffer data (required for accessor reads)
	// -------------------------------------------------------------------------

	if (!GltfLoaderInternal::LoadGltfBuffers(options, data, pathStr, result))
	{
		return result;
	}

	// -------------------------------------------------------------------------
	// Validate parsed data
	// -------------------------------------------------------------------------

	GltfLoaderInternal::ValidateGltf(data, pathStr);

	// -------------------------------------------------------------------------
	// Extract materials
	// -------------------------------------------------------------------------

	GltfLoaderInternal::ExtractMaterials(data, gltfDirectory, result.materials, result.texturePaths);

	// If the file has no materials, insert a default so meshes always have
	// a valid material index (0).
	GltfLoaderInternal::EnsureDefaultMaterial(result);

	// -------------------------------------------------------------------------
	// Count total primitives for reservation
	// -------------------------------------------------------------------------

	const std::size_t totalPrimitives = GltfLoaderInternal::CountTotalPrimitives(data);

	result.meshes.reserve(totalPrimitives);
	result.transforms.reserve(totalPrimitives);
	result.materialIndices.reserve(totalPrimitives);

	// -------------------------------------------------------------------------
	// Extract meshes from node hierarchy
	// -------------------------------------------------------------------------

	GltfLoaderInternal::ExtractMeshesFromNodes(data, result);

	// -------------------------------------------------------------------------
	// Finalize
	// -------------------------------------------------------------------------

	result.bSuccess = true;

	LOG_INFO(
	    std::format(
	        "GltfLoader: Loaded '{}' — {} meshes, {} materials, {} textures",
	        filePath.filename().string(),
	        result.meshes.size(),
	        result.materials.size(),
	        result.texturePaths.size()));

	return result;
}
