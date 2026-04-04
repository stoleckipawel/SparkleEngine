#include "PCH.h"

#include "Assets/Import/SceneImportPostProcessor.h"

#include "FileSystemUtils.h"

#include <algorithm>
#include <format>
#include <unordered_set>

void SceneImportPostProcessor::Finalize(SceneImportResult& result)
{
	NormalizeResultShape(result);
	NormalizeMaterialNamesAndTextures(result);
	DeduplicateMaterials(result);
	SanitizeMaterialOffsets(result);
	AccumulateStats(result);
}

void SceneImportPostProcessor::NormalizeResultShape(SceneImportResult& result)
{
	const std::string importerName = result.stats.importerName.empty() ? std::string("SceneImporter") : result.stats.importerName;

	if (result.transforms.size() < result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} transforms for {} meshes; missing transforms will use identity",
		        importerName,
		        result.transforms.size(),
		        result.meshes.size()));
		result.transforms.resize(result.meshes.size());
	}
	else if (result.transforms.size() > result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} transforms for {} meshes; extra transforms will be discarded",
		        importerName,
		        result.transforms.size(),
		        result.meshes.size()));
		result.transforms.resize(result.meshes.size());
	}

	if (result.materialOffsets.size() < result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} material offsets for {} meshes; missing offsets will use the default material",
		        importerName,
		        result.materialOffsets.size(),
		        result.meshes.size()));
		result.materialOffsets.resize(result.meshes.size(), 0);
	}
	else if (result.materialOffsets.size() > result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} material offsets for {} meshes; extra offsets will be discarded",
		        importerName,
		        result.materialOffsets.size(),
		        result.meshes.size()));
		result.materialOffsets.resize(result.meshes.size());
	}
}

void SceneImportPostProcessor::NormalizeMaterialNamesAndTextures(SceneImportResult& result)
{
	const std::string importerName = result.stats.importerName.empty() ? std::string("SceneImporter") : result.stats.importerName;

	for (std::size_t materialIndex = 0; materialIndex < result.materials.size(); ++materialIndex)
	{
		MaterialDesc& materialDesc = result.materials[materialIndex];
		if (materialDesc.name.empty())
		{
			materialDesc.name = std::format("ImportedMaterial_{}", materialIndex);
			result.AddWarning(
			    std::format("{}: Material {} was unnamed and has been renamed to '{}'", importerName, materialIndex, materialDesc.name));
		}

		NormalizeOptionalTexturePath(materialDesc.albedoTexture, importerName, materialDesc.name, "albedo", result);
		NormalizeOptionalTexturePath(materialDesc.normalTexture, importerName, materialDesc.name, "normal", result);
		NormalizeOptionalTexturePath(materialDesc.metallicRoughnessTexture, importerName, materialDesc.name, "metallic-roughness", result);
		NormalizeOptionalTexturePath(materialDesc.occlusionTexture, importerName, materialDesc.name, "occlusion", result);
		NormalizeOptionalTexturePath(materialDesc.emissiveTexture, importerName, materialDesc.name, "emissive", result);
	}
}

void SceneImportPostProcessor::DeduplicateMaterials(SceneImportResult& result)
{
	std::vector<std::uint32_t> materialRemap(result.materials.size(), 0);
	std::vector<MaterialDesc> deduplicatedMaterials;
	deduplicatedMaterials.reserve(result.materials.size());

	for (std::size_t materialIndex = 0; materialIndex < result.materials.size(); ++materialIndex)
	{
		const MaterialDesc& materialDesc = result.materials[materialIndex];
		auto existingIt = std::find_if(
		    deduplicatedMaterials.begin(),
		    deduplicatedMaterials.end(),
		    [&materialDesc](const MaterialDesc& existingMaterial)
		    {
			    return AreMaterialsEquivalent(existingMaterial, materialDesc);
		    });

		if (existingIt == deduplicatedMaterials.end())
		{
			materialRemap[materialIndex] = static_cast<std::uint32_t>(deduplicatedMaterials.size());
			deduplicatedMaterials.push_back(materialDesc);
			continue;
		}

		materialRemap[materialIndex] = static_cast<std::uint32_t>(std::distance(deduplicatedMaterials.begin(), existingIt));
		++result.stats.deduplicatedMaterialCount;
	}

	if (result.stats.deduplicatedMaterialCount == 0)
	{
		return;
	}

	for (std::uint32_t& materialOffset : result.materialOffsets)
	{
		if (materialOffset < materialRemap.size())
		{
			materialOffset = materialRemap[materialOffset];
		}
	}

	result.materials = std::move(deduplicatedMaterials);
}

void SceneImportPostProcessor::SanitizeMaterialOffsets(SceneImportResult& result)
{
	const std::string importerName = result.stats.importerName.empty() ? std::string("SceneImporter") : result.stats.importerName;

	for (std::size_t meshIndex = 0; meshIndex < result.materialOffsets.size(); ++meshIndex)
	{
		if (result.materials.empty())
		{
			result.materialOffsets[meshIndex] = 0;
			continue;
		}

		if (result.materialOffsets[meshIndex] >= result.materials.size())
		{
			result.AddWarning(
			    std::format(
			        "{}: Mesh {} references invalid material index {} and will use the default material",
			        importerName,
			        meshIndex,
			        result.materialOffsets[meshIndex]));
			result.materialOffsets[meshIndex] = 0;
		}
	}
}

void SceneImportPostProcessor::AccumulateStats(SceneImportResult& result)
{
	std::unordered_set<std::filesystem::path> uniqueTexturePaths;
	auto countTexturePath = [&uniqueTexturePaths, &result](const std::optional<std::filesystem::path>& texturePath)
	{
		if (!texturePath)
		{
			return;
		}

		if (!uniqueTexturePaths.insert(*texturePath).second)
		{
			++result.stats.duplicateTexturePathCount;
		}
	};

	for (const MaterialDesc& materialDesc : result.materials)
	{
		countTexturePath(materialDesc.albedoTexture);
		countTexturePath(materialDesc.normalTexture);
		countTexturePath(materialDesc.metallicRoughnessTexture);
		countTexturePath(materialDesc.occlusionTexture);
		countTexturePath(materialDesc.emissiveTexture);
	}

	result.stats.uniqueTexturePathCount = uniqueTexturePaths.size();

	for (const MeshData& meshData : result.meshes)
	{
		result.stats.totalVertices += meshData.vertices.size();
		result.stats.totalIndices += meshData.indices.size();
		result.stats.estimatedMeshBytes += meshData.GetVertexBufferSize() + meshData.GetIndexBufferSize();
	}

	constexpr std::size_t kLargeSceneMeshDataWarningBytes = 256ull * 1024ull * 1024ull;
	if (result.stats.estimatedMeshBytes >= kLargeSceneMeshDataWarningBytes)
	{
		const std::string importerName = result.stats.importerName.empty() ? std::string("SceneImporter") : result.stats.importerName;
		result.AddWarning(
		    std::format(
		        "{}: Imported mesh data consumes {:.2f} MiB before runtime upload; validate large-scene memory pressure",
		        importerName,
		        static_cast<double>(result.stats.estimatedMeshBytes) / (1024.0 * 1024.0)));
	}
}

bool SceneImportPostProcessor::AreMaterialsEquivalent(const MaterialDesc& lhs, const MaterialDesc& rhs) noexcept
{
	return lhs.name == rhs.name && lhs.baseColor.x == rhs.baseColor.x && lhs.baseColor.y == rhs.baseColor.y &&
	       lhs.baseColor.z == rhs.baseColor.z && lhs.baseColor.w == rhs.baseColor.w && lhs.metallic == rhs.metallic &&
	       lhs.roughness == rhs.roughness && lhs.f0 == rhs.f0 && lhs.emissiveColor.x == rhs.emissiveColor.x &&
	       lhs.emissiveColor.y == rhs.emissiveColor.y && lhs.emissiveColor.z == rhs.emissiveColor.z && lhs.alphaMode == rhs.alphaMode &&
	       lhs.alphaCutoff == rhs.alphaCutoff && lhs.albedoTexture == rhs.albedoTexture && lhs.normalTexture == rhs.normalTexture &&
	       lhs.metallicRoughnessTexture == rhs.metallicRoughnessTexture && lhs.occlusionTexture == rhs.occlusionTexture &&
	       lhs.emissiveTexture == rhs.emissiveTexture;
}

void SceneImportPostProcessor::NormalizeOptionalTexturePath(
    std::optional<std::filesystem::path>& texturePath,
    std::string_view importerName,
    std::string_view materialName,
    std::string_view slotName,
    SceneImportResult& result)
{
	if (!texturePath)
	{
		return;
	}

	const std::filesystem::path normalizedPath = Filesystem::NormalizePath(*texturePath);
	if (normalizedPath.empty())
	{
		result.AddWarning(
		    std::format(
		        "{}: Material '{}' has an invalid {} texture path '{}' and it will be ignored",
		        importerName,
		        materialName,
		        slotName,
		        texturePath->string()));
		texturePath.reset();
		return;
	}

	*texturePath = normalizedPath;
}