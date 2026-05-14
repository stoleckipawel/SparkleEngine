#include "PCH.h"

#include "Fbx/FbxMaterialImporter.h"

#include "Core/Public/Paths/PathUtils.h"

#include <format>

static const auto g_fbxMaterialImporterLogger = Logging::GetOrCreateLogger("Tools.SourceImportAdapters.Fbx");

void FbxMaterialImporter::ImportMaterials(const aiScene& scene, const std::filesystem::path& sourceDirectory, SourceImportResult& result)
{
	for (unsigned int materialIndex = 0; materialIndex < scene.mNumMaterials; ++materialIndex)
	{
		result.scene.materials.push_back(ExtractMaterial(*scene.mMaterials[materialIndex], materialIndex, sourceDirectory, result));
	}
}

ImportedMaterial FbxMaterialImporter::ExtractMaterial(
    const aiMaterial& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    SourceImportResult& result)
{
	ImportedMaterial importedMaterial;
	CollectMaterialWarnings(material, materialIndex, result);
	ApplyMaterialProperties(material, importedMaterial);
	ApplyTextureMappings(material, materialIndex, sourceDirectory, importedMaterial, result);
	return importedMaterial;
}

void FbxMaterialImporter::CollectMaterialWarnings(const aiMaterial& material, ImportedMaterialIndex materialIndex, SourceImportResult& result)
{
	int shadingModel = 0;
	if (material.Get(AI_MATKEY_SHADING_MODEL, shadingModel) != AI_SUCCESS)
	{
		return;
	}

	const aiShadingMode shadingMode = static_cast<aiShadingMode>(shadingModel);
	if (shadingMode == aiShadingMode_NoShading || shadingMode == aiShadingMode_Flat || shadingMode == aiShadingMode_Gouraud ||
	    shadingMode == aiShadingMode_Phong || shadingMode == aiShadingMode_Blinn || shadingMode == aiShadingMode_Unlit ||
	    shadingMode == aiShadingMode_PBR_BRDF)
	{
		return;
	}

	SPDLOG_LOGGER_WARN(
	    g_fbxMaterialImporterLogger,
	    "{}",
	    std::format(
	        "FbxImporter: Material handle {} uses unsupported shading model {} and will be approximated with Sparkle PBR defaults",
	        materialIndex,
	        shadingModel));
}

void FbxMaterialImporter::ApplyMaterialProperties(const aiMaterial& material, ImportedMaterial& importedMaterial)
{
	aiColor4D baseColor;
	if (aiGetMaterialColor(&material, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS ||
	    aiGetMaterialColor(&material, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS)
	{
		importedMaterial.baseColor = DirectX::XMFLOAT4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
	}

	ai_real opacity = 1.0f;
	if (material.Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
	{
		importedMaterial.baseColor.w = static_cast<float>(opacity);
		if (importedMaterial.baseColor.w < 1.0f)
		{
			importedMaterial.alphaMode = ImportedAlphaMode::Blend;
		}
	}

	aiColor4D emissiveColor;
	if (aiGetMaterialColor(&material, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor) == AI_SUCCESS)
	{
		importedMaterial.emissiveColor = DirectX::XMFLOAT3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
	}

	ai_real metallic = 0.0f;
	if (material.Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
	{
		importedMaterial.metallic = static_cast<float>(metallic);
	}

	ai_real roughness = 0.5f;
	if (material.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
	{
		importedMaterial.roughness = static_cast<float>(roughness);
	}
}

void FbxMaterialImporter::ApplyTextureMappings(
    const aiMaterial& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    ImportedMaterial& importedMaterial,
    SourceImportResult& result)
{
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::Diffuse, importedMaterial, result);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::NormalMap, importedMaterial, result);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::Roughness, importedMaterial, result);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::Metallic, importedMaterial, result);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::AmbientOcclusion, importedMaterial, result);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::Emissive, importedMaterial, result);
}

void FbxMaterialImporter::AssignTextureByType(
    const aiMaterial& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    TextureGroup textureGroup,
    ImportedMaterial& importedMaterial,
    SourceImportResult& result)
{
	switch (textureGroup)
	{
		case TextureGroup::Diffuse:
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_BASE_COLOR, "base-color", result));

			if (importedMaterial.textureSources.empty())
			{
				SetTextureSource(
				    importedMaterial,
				    textureGroup,
				    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_DIFFUSE, "diffuse", result));
			}
			break;

		case TextureGroup::NormalMap:
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_NORMALS, "normal", result));

			if (importedMaterial.textureSources.empty() || importedMaterial.textureSources.back().textureGroup != TextureGroup::NormalMap)
			{
				SetTextureSource(
				    importedMaterial,
				    textureGroup,
				    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_HEIGHT, "height", result));
			}
			break;

		case TextureGroup::Roughness:
		{
			const std::optional<std::filesystem::path> roughnessTexture =
			    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_DIFFUSE_ROUGHNESS, "roughness", result);
			SetTextureSource(importedMaterial, textureGroup, roughnessTexture, TextureChannelMask::Red);
			if (roughnessTexture)
			{
				importedMaterial.roughness = 1.0f;
			}
			break;
		}

		case TextureGroup::Metallic:
		{
			std::optional<std::filesystem::path> metallicTexture =
			    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_METALNESS, "metallic", result);
			if (!metallicTexture)
			{
				metallicTexture = ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_SPECULAR, "specular", result);
			}

			SetTextureSource(importedMaterial, textureGroup, metallicTexture, TextureChannelMask::Red);
			if (metallicTexture)
			{
				importedMaterial.metallic = 1.0f;
			}
			break;
		}

		case TextureGroup::AmbientOcclusion:
		{
			std::optional<std::filesystem::path> occlusionTexture =
			    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_AMBIENT_OCCLUSION, "occlusion", result);
			if (!occlusionTexture)
			{
				occlusionTexture =
				    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_SPECULAR, "specular", result);
			}

			SetTextureSource(importedMaterial, textureGroup, occlusionTexture, TextureChannelMask::Red);
			break;
		}

		case TextureGroup::Emissive:
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(material, materialIndex, sourceDirectory, aiTextureType_EMISSIVE, "emissive", result));
			break;

		case TextureGroup::SubsurfaceColor:
		case TextureGroup::SubsurfaceStrength:
		case TextureGroup::Default:
		case TextureGroup::HdrColor:
			break;
	}
}

void FbxMaterialImporter::SetTextureSource(
	ImportedMaterial& importedMaterial,
    TextureGroup textureGroup,
    const std::optional<std::filesystem::path>& texturePath,
	TextureChannelMask channelMask)
{
	if (!texturePath)
	{
		return;
	}

	importedMaterial.textureSources.push_back({textureGroup, *texturePath, channelMask});
}

std::optional<std::filesystem::path> FbxMaterialImporter::ResolveTexturePath(
    const aiMaterial& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    aiTextureType textureType,
    std::string_view slotName,
    SourceImportResult& result)
{
	const unsigned int textureCount = material.GetTextureCount(textureType);
	if (textureCount == 0)
	{
		return std::nullopt;
	}

	if (textureCount > 1)
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxMaterialImporterLogger,
		    "{}",
		    std::format(
		        "FbxImporter: Material handle {} has multiple {} textures and only the first will be used",
		        materialIndex,
		        slotName));
	}

	aiString texturePath;
	if (material.GetTexture(textureType, 0, &texturePath) != AI_SUCCESS)
	{
		return std::nullopt;
	}

	const std::string texturePathString = texturePath.C_Str();
	if (texturePathString.empty())
	{
		return std::nullopt;
	}

	if (texturePathString[0] == '*')
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxMaterialImporterLogger,
		    "{}",
		    std::format(
		        "FbxImporter: Material handle {} uses embedded {} texture '{}' which is not supported yet",
		        materialIndex,
		        slotName,
		        texturePathString));
		return std::nullopt;
	}

	const std::optional<std::filesystem::path> resolvedTexturePath =
	    Paths::ResolveRelativePath(sourceDirectory, std::filesystem::path(texturePathString));
	if (!resolvedTexturePath)
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxMaterialImporterLogger,
		    "{}",
		    std::format(
		        "FbxImporter: Material handle {} has an invalid {} texture path '{}' and it will be ignored",
		        materialIndex,
		        slotName,
		        texturePathString));
		return std::nullopt;
	}

	return NormalizeTexturePath(*resolvedTexturePath, materialIndex, slotName);
}

std::optional<std::filesystem::path> FbxMaterialImporter::NormalizeTexturePath(
    std::filesystem::path texturePath,
    ImportedMaterialIndex materialIndex,
    std::string_view slotName)
{
	const std::filesystem::path normalizedTexturePath = Paths::Normalize(texturePath);
	if (normalizedTexturePath.empty())
	{
		SPDLOG_LOGGER_WARN(
		    g_fbxMaterialImporterLogger,
		    "{}",
		    std::format(
		        "FbxImporter: Material handle {} has an invalid {} texture path '{}' and it will be ignored",
		        materialIndex,
		        slotName,
		        texturePath.string()));
		return std::nullopt;
	}

	return normalizedTexturePath;
}


