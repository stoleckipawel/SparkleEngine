#include "PCH.h"

#include "Assets/Import/SceneImportPostProcessor.h"

#include "FileSystemUtils.h"

#include <format>

void SceneImportPostProcessor::Finalize(SceneImportResult& result)
{
	NormalizeTransformCount(result);
	NormalizeMaterialOffsetCount(result);
	NormalizeMaterialTextures(result);
	EnsureMaterialNames(result);
	SanitizeMaterialOffsets(result);
	AccumulateStats(result);
}

void SceneImportPostProcessor::NormalizeTransformCount(SceneImportResult& result)
{
	if (result.transforms.size() < result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} transforms for {} meshes; missing transforms will use identity",
		        result.stats.importerName,
		        result.transforms.size(),
		        result.meshes.size()));

		result.transforms.resize(result.meshes.size());
	}
	else if (result.transforms.size() > result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} transforms for {} meshes; extra transforms will be discarded",
		        result.stats.importerName,
		        result.transforms.size(),
		        result.meshes.size()));

		result.transforms.resize(result.meshes.size());
	}
}

void SceneImportPostProcessor::NormalizeMaterialOffsetCount(SceneImportResult& result)
{
	if (result.materialOffsets.size() < result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} material offsets for {} meshes; missing offsets will use the default material",
		        result.stats.importerName,
		        result.materialOffsets.size(),
		        result.meshes.size()));

		result.materialOffsets.resize(result.meshes.size(), 0);
	}
	else if (result.materialOffsets.size() > result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} material offsets for {} meshes; extra offsets will be discarded",
		        result.stats.importerName,
		        result.materialOffsets.size(),
		        result.meshes.size()));

		result.materialOffsets.resize(result.meshes.size());
	}
}

void SceneImportPostProcessor::NormalizeMaterialTextures(SceneImportResult& result)
{
	for (std::size_t materialIndex = 0; materialIndex < result.materials.size(); ++materialIndex)
	{
		MaterialDesc& materialDesc = result.materials[materialIndex];
		NormalizeOptionalTexturePath(materialDesc.albedoTexture, materialDesc.name, "albedo", result);
		NormalizeOptionalTexturePath(materialDesc.normalTexture, materialDesc.name, "normal", result);
		NormalizeOptionalTexturePath(materialDesc.metallicRoughnessTexture, materialDesc.name, "metallic-roughness", result);
		NormalizeOptionalTexturePath(materialDesc.occlusionTexture, materialDesc.name, "occlusion", result);
		NormalizeOptionalTexturePath(materialDesc.emissiveTexture, materialDesc.name, "emissive", result);
	}
}

void SceneImportPostProcessor::EnsureMaterialNames(SceneImportResult& result)
{
	for (std::size_t materialIndex = 0; materialIndex < result.materials.size(); ++materialIndex)
	{
		MaterialDesc& materialDesc = result.materials[materialIndex];
		if (materialDesc.name.empty())
		{
			materialDesc.name = std::format("ImportedMaterial_{}", materialIndex);
			result.AddWarning(
			    std::format("{}: Material {} was unnamed and has been renamed to '{}'", result.stats.importerName, materialIndex, materialDesc.name));
		}
	}
}

void SceneImportPostProcessor::SanitizeMaterialOffsets(SceneImportResult& result)
{
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
			        result.stats.importerName,
			        meshIndex,
			        result.materialOffsets[meshIndex]));
			result.materialOffsets[meshIndex] = 0;
		}
	}
}

void SceneImportPostProcessor::AccumulateStats(SceneImportResult& result)
{
	for (const MeshData& meshData : result.meshes)
	{
		result.stats.totalVertices += meshData.vertices.size();
		result.stats.totalIndices += meshData.indices.size();
		result.stats.estimatedMeshBytes += meshData.GetVertexBufferSize() + meshData.GetIndexBufferSize();
	}

	constexpr std::size_t kLargeSceneMeshDataWarningBytes = 256ull * 1024ull * 1024ull;
	if (result.stats.estimatedMeshBytes >= kLargeSceneMeshDataWarningBytes)
	{
		result.AddWarning(
		    std::format(
		        "{}: Imported mesh data consumes {:.2f} MiB before runtime upload; validate large-scene memory pressure",
		        result.stats.importerName,
		        static_cast<double>(result.stats.estimatedMeshBytes) / (1024.0 * 1024.0)));
	}
}

void SceneImportPostProcessor::NormalizeOptionalTexturePath(
    std::optional<std::filesystem::path>& texturePath,
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
		        result.stats.importerName,
		        materialName,
		        slotName,
		        texturePath->string()));
		texturePath.reset();
		return;
	}

	*texturePath = normalizedPath;
}