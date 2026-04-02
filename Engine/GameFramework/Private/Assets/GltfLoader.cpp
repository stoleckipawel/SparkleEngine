#define CGLTF_IMPLEMENTATION
#include "PCH.h"

#include "GameFramework/Public/Assets/GltfLoader.h"

#include "Assets/SceneImportUtilities.h"

#include <cgltf.h>

#include <format>

using namespace DirectX;

GltfLoader::CgltfGuard::~CgltfGuard()
{
	cgltf_free(ptr);
}

bool GltfLoader::ValidateInputPath(const std::filesystem::path& filePath, SceneImportResult& result)
{
	if (std::filesystem::exists(filePath))
	{
		return true;
	}

	result.errorMessage = std::format("GltfLoader: File not found: {}", filePath.string());
	LOG_ERROR(result.errorMessage);
	return false;
}

bool GltfLoader::ParseGltfFile(
	cgltf_options& options,
	const std::string& pathStr,
	cgltf_data*& outData,
	SceneImportResult& result)
{
	const cgltf_result parseResult = cgltf_parse_file(&options, pathStr.c_str(), &outData);
	if (parseResult == cgltf_result_success)
	{
		return true;
	}

	result.errorMessage = std::format(
	    "GltfLoader: Failed to parse '{}' (cgltf error {})",
	    pathStr,
	    static_cast<int>(parseResult));
	LOG_ERROR(result.errorMessage);
	return false;
}

bool GltfLoader::LoadGltfBuffers(
	cgltf_options& options,
	cgltf_data* data,
	const std::string& pathStr,
	SceneImportResult& result)
{
	const cgltf_result bufferResult = cgltf_load_buffers(&options, data, pathStr.c_str());
	if (bufferResult == cgltf_result_success)
	{
		return true;
	}

	result.errorMessage = std::format(
	    "GltfLoader: Failed to load buffers for '{}' (cgltf error {})",
	    pathStr,
	    static_cast<int>(bufferResult));
	LOG_ERROR(result.errorMessage);
	return false;
}

void GltfLoader::ValidateGltf(cgltf_data* data, const std::string& pathStr, SceneImportResult& result)
{
	const cgltf_result validateResult = cgltf_validate(data);
	if (validateResult != cgltf_result_success)
	{
		result.AddWarning(std::format(
		    "GltfLoader: Validation warnings for '{}' (cgltf error {})",
		    pathStr,
		    static_cast<int>(validateResult)));
	}
}

void GltfLoader::CollectSceneWarnings(const cgltf_data* data, SceneImportResult& result)
{
	if (data->animations_count > 0)
	{
		result.AddWarning(std::format(
		    "GltfLoader: {} animations are present and will be ignored",
		    data->animations_count));
	}

	if (data->variants_count > 0)
	{
		result.AddWarning(std::format(
		    "GltfLoader: {} material variants are present and will be ignored",
		    data->variants_count));
	}

	std::size_t cameraNodeCount = 0;
	std::size_t lightNodeCount = 0;
	std::size_t skinnedNodeCount = 0;
	std::size_t weightedNodeCount = 0;
	std::size_t instancedNodeCount = 0;

	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		cameraNodeCount += node.camera != nullptr ? 1u : 0u;
		lightNodeCount += node.light != nullptr ? 1u : 0u;
		skinnedNodeCount += node.skin != nullptr ? 1u : 0u;
		weightedNodeCount += node.weights_count > 0 ? 1u : 0u;
		instancedNodeCount += node.has_mesh_gpu_instancing ? 1u : 0u;
	}

	if (cameraNodeCount > 0)
	{
		result.AddWarning(std::format(
		    "GltfLoader: {} nodes contain cameras and they will be ignored",
		    cameraNodeCount));
	}

	if (lightNodeCount > 0)
	{
		result.AddWarning(std::format(
		    "GltfLoader: {} nodes contain lights and they will be ignored",
		    lightNodeCount));
	}

	if (skinnedNodeCount > 0)
	{
		result.AddWarning(std::format(
		    "GltfLoader: {} skinned nodes are present and will be imported as static data only",
		    skinnedNodeCount));
	}

	if (weightedNodeCount > 0)
	{
		result.AddWarning(std::format(
		    "GltfLoader: {} weighted nodes are present and morph weights will be ignored",
		    weightedNodeCount));
	}

	if (instancedNodeCount > 0)
	{
		result.AddWarning(std::format(
		    "GltfLoader: {} nodes use mesh GPU instancing and will be flattened to regular mesh instances",
		    instancedNodeCount));
	}
}

std::size_t GltfLoader::CountTotalPrimitives(const cgltf_data* data)
{
	std::size_t totalPrimitives = 0;
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (node.mesh)
		{
			totalPrimitives += node.mesh->primitives_count;
		}
	}

	return totalPrimitives;
}

std::string GltfLoader::BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex)
{
	const std::string nodeName = node.name ? node.name : std::string("<unnamed-node>");
	return std::format("node '{}' primitive {}", nodeName, primitiveIndex);
}

void GltfLoader::ExtractMeshesFromNodes(const cgltf_data* data, SceneImportResult& result)
{
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (!node.mesh)
		{
			continue;
		}

		const XMMATRIX worldTransform = ComputeNodeWorldTransform(&node);

		for (cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count; ++primitiveIndex)
		{
			const cgltf_primitive& primitive = node.mesh->primitives[primitiveIndex];
			const std::string primitiveLabel = BuildPrimitiveLabel(node, primitiveIndex);

			if (primitive.type != cgltf_primitive_type_triangles)
			{
				result.AddWarning(std::format(
				    "GltfLoader: Skipping {} because only triangle primitives are supported",
				    primitiveLabel));
				continue;
			}

			if (primitive.targets_count > 0)
			{
				result.AddWarning(std::format(
				    "GltfLoader: {} contains morph targets which will be ignored",
				    primitiveLabel));
			}

			if (primitive.has_draco_mesh_compression)
			{
				result.AddWarning(std::format(
				    "GltfLoader: Skipping {} because Draco-compressed primitives are not supported yet",
				    primitiveLabel));
				continue;
			}

			if (primitive.mappings_count > 0)
			{
				result.AddWarning(std::format(
				    "GltfLoader: {} contains material variant mappings which will be ignored",
				    primitiveLabel));
			}

			MeshData meshData = ExtractPrimitive(primitive);
			if (!meshData.IsValid())
			{
				result.AddWarning(std::format(
				    "GltfLoader: Skipping {} because vertex or index data is incomplete",
				    primitiveLabel));
				continue;
			}

			result.materialOffsets.push_back(ResolveMaterialOffset(primitive, data));
			result.transforms.emplace_back(worldTransform);
			result.meshes.push_back(std::move(meshData));
		}
	}
}

template <typename T> T GltfLoader::ReadAccessorElement(const cgltf_accessor* accessor, std::size_t index)
{
	T element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(T) / sizeof(float));
	}

	return element;
}

const cgltf_accessor* GltfLoader::FindAttribute(const cgltf_primitive& primitive, int type)
{
	for (cgltf_size attributeIndex = 0; attributeIndex < primitive.attributes_count; ++attributeIndex)
	{
		if (primitive.attributes[attributeIndex].type == static_cast<cgltf_attribute_type>(type))
		{
			return primitive.attributes[attributeIndex].data;
		}
	}

	return nullptr;
}

void GltfLoader::ReadIndices(const cgltf_accessor* accessor, std::vector<std::uint32_t>& outIndices)
{
	if (!accessor)
	{
		return;
	}

	outIndices.resize(accessor->count);
	for (cgltf_size index = 0; index < accessor->count; ++index)
	{
		outIndices[index] = static_cast<std::uint32_t>(cgltf_accessor_read_index(accessor, index));
	}
}

XMMATRIX GltfLoader::ComputeNodeWorldTransform(const cgltf_node* node)
{
	XMMATRIX worldTransform = XMMatrixIdentity();

	const cgltf_node* nodeChain[64];
	int depth = 0;
	for (const cgltf_node* currentNode = node; currentNode != nullptr && depth < 64; currentNode = currentNode->parent)
	{
		nodeChain[depth++] = currentNode;
	}

	for (int chainIndex = depth - 1; chainIndex >= 0; --chainIndex)
	{
		float localMatrix[16];
		cgltf_node_transform_local(nodeChain[chainIndex], localMatrix);
		const XMMATRIX localTransform = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(localMatrix));
		worldTransform = XMMatrixMultiply(worldTransform, localTransform);
	}

	return worldTransform;
}

std::optional<std::filesystem::path> GltfLoader::ResolveTexturePath(
	const cgltf_texture_view& textureView,
	const std::filesystem::path& gltfDirectory,
	std::string_view materialName,
	std::string_view slotName,
	SceneImportResult& result)
{
	if (!textureView.texture)
	{
		return std::nullopt;
	}

	const cgltf_texture& texture = *textureView.texture;
	if (texture.image && texture.image->uri)
	{
		return SceneImportUtilities::NormalizeImportedTexturePath(gltfDirectory, std::filesystem::path(texture.image->uri));
	}

	if (texture.image && texture.image->buffer_view)
	{
		result.AddWarning(std::format(
		    "GltfLoader: Material '{}' uses an embedded {} texture which is not supported yet",
		    materialName,
		    slotName));
		return std::nullopt;
	}

	if (texture.has_basisu || texture.has_webp)
	{
		result.AddWarning(std::format(
		    "GltfLoader: Material '{}' uses {} texture sources that are not supported by the runtime importer yet",
		    materialName,
		    slotName));
	}

	return std::nullopt;
}

void GltfLoader::AppendUnsupportedMaterialWarnings(
	const cgltf_material& material,
	std::string_view materialName,
	SceneImportResult& result)
{
	std::string unsupportedFeatures;
	auto appendFeature = [&unsupportedFeatures](std::string_view featureName)
	{
		if (!unsupportedFeatures.empty())
		{
			unsupportedFeatures += ", ";
		}

		unsupportedFeatures += featureName;
	};

	if (material.has_pbr_specular_glossiness)
	{
		appendFeature("KHR_materials_pbrSpecularGlossiness");
	}

	if (material.unlit)
	{
		appendFeature("KHR_materials_unlit");
	}

	if (material.has_clearcoat)
	{
		appendFeature("KHR_materials_clearcoat");
	}

	if (material.has_transmission)
	{
		appendFeature("KHR_materials_transmission");
	}

	if (material.has_volume)
	{
		appendFeature("KHR_materials_volume");
	}

	if (material.has_ior)
	{
		appendFeature("KHR_materials_ior");
	}

	if (material.has_specular)
	{
		appendFeature("KHR_materials_specular");
	}

	if (material.has_sheen)
	{
		appendFeature("KHR_materials_sheen");
	}

	if (material.has_emissive_strength)
	{
		appendFeature("KHR_materials_emissive_strength");
	}

	if (material.has_iridescence)
	{
		appendFeature("KHR_materials_iridescence");
	}

	if (material.has_diffuse_transmission)
	{
		appendFeature("KHR_materials_diffuse_transmission");
	}

	if (material.has_anisotropy)
	{
		appendFeature("KHR_materials_anisotropy");
	}

	if (material.has_dispersion)
	{
		appendFeature("KHR_materials_dispersion");
	}

	if (!unsupportedFeatures.empty())
	{
		result.AddWarning(std::format(
		    "GltfLoader: Material '{}' uses unsupported glTF material features [{}] and will be approximated with Sparkle PBR defaults",
		    materialName,
		    unsupportedFeatures));
	}
}

void GltfLoader::ExtractMaterials(
	const cgltf_data* data,
	const std::filesystem::path& gltfDirectory,
	SceneImportResult& result)
{
	result.materials.reserve(data->materials_count);

	for (cgltf_size materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
	{
		const cgltf_material& material = data->materials[materialIndex];
		MaterialDesc materialDesc = SceneImportUtilities::CreateMaterialDesc(
		    material.name ? material.name : std::format("Material_{}", materialIndex));

		materialDesc.emissiveColor = XMFLOAT3(
		    material.emissive_factor[0],
		    material.emissive_factor[1],
		    material.emissive_factor[2]);
		materialDesc.alphaCutoff = material.alpha_cutoff;

		switch (material.alpha_mode)
		{
			case cgltf_alpha_mode_mask:
				materialDesc.alphaMode = AlphaMode::Mask;
				break;
			case cgltf_alpha_mode_blend:
				materialDesc.alphaMode = AlphaMode::Blend;
				break;
			case cgltf_alpha_mode_opaque:
			default:
				materialDesc.alphaMode = AlphaMode::Opaque;
				break;
		}

		AppendUnsupportedMaterialWarnings(material, materialDesc.name, result);

		if (material.has_pbr_metallic_roughness)
		{
			const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;
			materialDesc.baseColor = XMFLOAT4(
			    pbr.base_color_factor[0],
			    pbr.base_color_factor[1],
			    pbr.base_color_factor[2],
			    pbr.base_color_factor[3]);
			materialDesc.metallic = pbr.metallic_factor;
			materialDesc.roughness = pbr.roughness_factor;

			SceneImportUtilities::SetMaterialTexture(
			    materialDesc,
			    ImportedTextureSemantic::Albedo,
			    ResolveTexturePath(
			        pbr.base_color_texture,
			        gltfDirectory,
			        materialDesc.name,
			        "base-color",
			        result));

			SceneImportUtilities::SetMaterialTexture(
			    materialDesc,
			    ImportedTextureSemantic::MetallicRoughness,
			    ResolveTexturePath(
			        pbr.metallic_roughness_texture,
			        gltfDirectory,
			        materialDesc.name,
			        "metallic-roughness",
			        result));
		}
		else if (material.has_pbr_specular_glossiness)
		{
			const cgltf_pbr_specular_glossiness& specGloss = material.pbr_specular_glossiness;
			materialDesc.baseColor = XMFLOAT4(
			    specGloss.diffuse_factor[0],
			    specGloss.diffuse_factor[1],
			    specGloss.diffuse_factor[2],
			    specGloss.diffuse_factor[3]);
			materialDesc.metallic = 0.0f;
			materialDesc.roughness = 1.0f - specGloss.glossiness_factor;

			SceneImportUtilities::SetMaterialTexture(
			    materialDesc,
			    ImportedTextureSemantic::Albedo,
			    ResolveTexturePath(
			        specGloss.diffuse_texture,
			        gltfDirectory,
			        materialDesc.name,
			        "diffuse",
			        result));
		}

		SceneImportUtilities::SetMaterialTexture(
		    materialDesc,
		    ImportedTextureSemantic::Normal,
		    ResolveTexturePath(
		        material.normal_texture,
		        gltfDirectory,
		        materialDesc.name,
		        "normal",
		        result));

		SceneImportUtilities::SetMaterialTexture(
		    materialDesc,
		    ImportedTextureSemantic::Occlusion,
		    ResolveTexturePath(
		        material.occlusion_texture,
		        gltfDirectory,
		        materialDesc.name,
		        "occlusion",
		        result));

		SceneImportUtilities::SetMaterialTexture(
		    materialDesc,
		    ImportedTextureSemantic::Emissive,
		    ResolveTexturePath(
		        material.emissive_texture,
		        gltfDirectory,
		        materialDesc.name,
		        "emissive",
		        result));

		result.materials.push_back(std::move(materialDesc));
	}
}

std::uint32_t GltfLoader::ResolveMaterialOffset(const cgltf_primitive& primitive, const cgltf_data* data)
{
	if (!primitive.material)
	{
		return 0;
	}

	return static_cast<std::uint32_t>(primitive.material - data->materials);
}

MeshData GltfLoader::ExtractPrimitive(const cgltf_primitive& primitive)
{
	const cgltf_accessor* positions = FindAttribute(primitive, cgltf_attribute_type_position);
	const cgltf_accessor* normals = FindAttribute(primitive, cgltf_attribute_type_normal);
	const cgltf_accessor* texcoords = FindAttribute(primitive, cgltf_attribute_type_texcoord);
	const cgltf_accessor* tangents = FindAttribute(primitive, cgltf_attribute_type_tangent);

	if (!positions)
	{
		return {};
	}

	const std::uint32_t vertexCount = static_cast<std::uint32_t>(positions->count);
	MeshData meshData;
	meshData.Reserve(vertexCount, primitive.indices ? static_cast<std::uint32_t>(primitive.indices->count) : 0);
	meshData.vertices.resize(vertexCount);

	for (std::uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
	{
		VertexData& vertex = meshData.vertices[vertexIndex];
		vertex.position = ReadAccessorElement<XMFLOAT3>(positions, vertexIndex);
		vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};

		if (normals)
		{
			vertex.normal = ReadAccessorElement<XMFLOAT3>(normals, vertexIndex);
		}

		if (texcoords)
		{
			vertex.uv = ReadAccessorElement<XMFLOAT2>(texcoords, vertexIndex);
		}

		if (tangents)
		{
			vertex.tangent = ReadAccessorElement<XMFLOAT4>(tangents, vertexIndex);
		}
	}

	ReadIndices(primitive.indices, meshData.indices);
	return meshData;
}

SceneImportResult GltfLoader::Load(const std::filesystem::path& filePath)
{
	SceneImportResult result;

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

	ValidateGltf(data, pathStr, result);
	CollectSceneWarnings(data, result);
	ExtractMaterials(data, gltfDirectory, result);

	const std::size_t totalPrimitives = CountTotalPrimitives(data);
	result.Reserve(totalPrimitives);
	ExtractMeshesFromNodes(data, result);

	if (result.meshes.empty())
	{
		result.errorMessage = std::format(
		    "GltfLoader: No supported mesh primitives found in '{}'",
		    filePath.string());
		return result;
	}

	result.bSuccess = true;

	LOG_INFO(std::format(
	    "GltfLoader: Loaded '{}' — {} meshes, {} materials",
	    filePath.filename().string(),
	    result.meshes.size(),
	    result.materials.size()));

	return result;
}