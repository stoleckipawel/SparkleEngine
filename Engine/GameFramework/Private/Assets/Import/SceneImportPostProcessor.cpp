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
	if (result.transforms.size() < result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} transforms for {} meshes; missing transforms will use identity",
		        result.importerName,
		        result.transforms.size(),
		        result.meshes.size()));

		result.transforms.resize(result.meshes.size());
	}
	else if (result.transforms.size() > result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} transforms for {} meshes; extra transforms will be discarded",
		        result.importerName,
		        result.transforms.size(),
		        result.meshes.size()));

		result.transforms.resize(result.meshes.size());
	}
}

void SceneImportPostProcessor::NormalizeMaterialHandleCount(SceneImportResult& result)
{
	if (result.materialHandles.size() < result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} material handles for {} meshes; missing handles will use the default material",
		        result.importerName,
		        result.materialHandles.size(),
		        result.meshes.size()));

		result.materialHandles.resize(result.meshes.size(), MaterialHandle::Invalid());
	}
	else if (result.materialHandles.size() > result.meshes.size())
	{
		result.AddWarning(
		    std::format(
		        "{}: Import result contained {} material handles for {} meshes; extra handles will be discarded",
		        result.importerName,
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
			result.AddWarning(
			    std::format(
			        "{}: Mesh {} references invalid material handle {} and will use the default material",
			        result.importerName,
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
	if (!texturePath)
	{
		return;
	}

	const std::filesystem::path normalizedPath = Engine::Paths::Normalize(*texturePath);
	if (normalizedPath.empty())
	{
		result.AddWarning(
		    std::format(
		        "{}: Material '{}' has an invalid {} texture path '{}' and it will be ignored",
		        result.importerName,
		        materialName,
		        slotName,
		        texturePath->string()));
				
		texturePath.reset();
		return;
	}

	*texturePath = normalizedPath;
}