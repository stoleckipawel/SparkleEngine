#include "PCH.h"

#include "Assets/Import/SceneImportPostProcessor.h"

#include "Core/Public/Paths/PathUtils.h"

#include <format>

void SceneImportPostProcessor::Finalize(SceneImportResult& result)
{
	NormalizeTransformCount(result);
	NormalizeMaterialHandleCount(result);
	NormalizeMaterialTextures(result);
	SanitizeMaterialHandles(result);
}

void SceneImportPostProcessor::NormalizeTransformCount(SceneImportResult& result)
{
	const std::string_view importerName = GetSceneImporterTypeName(result.importerType);

	if (result.transforms.size() < result.meshes.size())
	{
		LOG_WARNING(std::format(
		    "{}: Import result contained {} transforms for {} meshes; missing transforms will use identity",
		    importerName,
		    result.transforms.size(),
		    result.meshes.size()));

		result.transforms.resize(result.meshes.size());
	}
	else if (result.transforms.size() > result.meshes.size())
	{
		LOG_WARNING(std::format(
		    "{}: Import result contained {} transforms for {} meshes; extra transforms will be discarded",
		    importerName,
		    result.transforms.size(),
		    result.meshes.size()));

		result.transforms.resize(result.meshes.size());
	}
}

void SceneImportPostProcessor::NormalizeMaterialHandleCount(SceneImportResult& result)
{
	const std::string_view importerName = GetSceneImporterTypeName(result.importerType);

	if (result.materialHandles.size() < result.meshes.size())
	{
		LOG_WARNING(std::format(
		    "{}: Import result contained {} material handles for {} meshes; missing handles will use the default material",
		    importerName,
		    result.materialHandles.size(),
		    result.meshes.size()));

		result.materialHandles.resize(result.meshes.size(), MaterialHandle::Invalid());
	}
	else if (result.materialHandles.size() > result.meshes.size())
	{
		LOG_WARNING(std::format(
		    "{}: Import result contained {} material handles for {} meshes; extra handles will be discarded",
		    importerName,
		    result.materialHandles.size(),
		    result.meshes.size()));

		result.materialHandles.resize(result.meshes.size());
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

void SceneImportPostProcessor::SanitizeMaterialHandles(SceneImportResult& result)
{
	const std::string_view importerName = GetSceneImporterTypeName(result.importerType);

	for (std::size_t meshIndex = 0; meshIndex < result.materialHandles.size(); ++meshIndex)
	{
		if (result.materials.empty())
		{
			result.materialHandles[meshIndex] = MaterialHandle::Invalid();
			continue;
		}

		const MaterialHandle materialHandle = result.materialHandles[meshIndex];
		if (!materialHandle.IsValid())
		{
			continue;
		}

		if (materialHandle.GetIndex() >= result.materials.size())
		{
			LOG_WARNING(std::format(
			    "{}: Mesh {} references invalid material handle {} and will use the default material",
			    importerName,
			    meshIndex,
			    materialHandle.GetIndex()));
			result.materialHandles[meshIndex] = MaterialHandle::Invalid();
		}
	}
}

void SceneImportPostProcessor::NormalizeOptionalTexturePath(
    std::optional<std::filesystem::path>& texturePath,
    std::string_view materialName,
    std::string_view slotName,
    SceneImportResult& result)
{
	const std::string_view importerName = GetSceneImporterTypeName(result.importerType);

	if (!texturePath)
	{
		return;
	}

	const std::filesystem::path normalizedPath = Engine::Paths::Normalize(*texturePath);
	if (normalizedPath.empty())
	{
		LOG_WARNING(std::format(
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