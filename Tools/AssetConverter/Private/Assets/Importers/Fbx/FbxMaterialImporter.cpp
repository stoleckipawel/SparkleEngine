#include "PCH.h"

#include "Assets/Importers/Fbx/FbxMaterialImporter.h"

#include "Core/Public/Paths/PathUtils.h"

#include <format>

static const auto g_fbxMaterialImporterLogger = Logging::GetOrCreateLogger("Tools.AssetConverter.Fbx");

void FbxMaterialImporter::ImportMaterials(const aiScene& scene, const std::filesystem::path& sourceDirectory, SceneImportResult& result)
{
	result.materialTextureSources.reserve(scene.mNumMaterials);
	for (unsigned int materialIndex = 0; materialIndex < scene.mNumMaterials; ++materialIndex)
	{
		std::vector<SceneImportResult::MaterialTextureSource> textureSources;
		result.materials.push_back(
		    ExtractMaterial(*scene.mMaterials[materialIndex], materialIndex, sourceDirectory, textureSources, result));
		result.materialTextureSources.push_back(std::move(textureSources));
	}
}

MaterialDesc FbxMaterialImporter::ExtractMaterial(
    const aiMaterial& material,
    unsigned int materialIndex,
    const std::filesystem::path& sourceDirectory,
    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
    SceneImportResult& result)
{
	const MaterialHandle materialHandle(materialIndex);
	MaterialDesc materialDesc;
	CollectMaterialWarnings(material, materialHandle, result);
	ApplyMaterialProperties(material, materialDesc);
	ApplyTextureMappings(material, materialHandle, sourceDirectory, materialDesc, outTextureSources, result);
	return materialDesc;
}

void FbxMaterialImporter::CollectMaterialWarnings(const aiMaterial& material, MaterialHandle materialHandle, SceneImportResult& result)
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
	        materialHandle.GetIndex(),
	        shadingModel));
}

void FbxMaterialImporter::ApplyMaterialProperties(const aiMaterial& material, MaterialDesc& materialDesc)
{
	aiColor4D baseColor;
	if (aiGetMaterialColor(&material, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS ||
	    aiGetMaterialColor(&material, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS)
	{
		materialDesc.baseColor = DirectX::XMFLOAT4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
	}

	ai_real opacity = 1.0f;
	if (material.Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
	{
		materialDesc.baseColor.w = static_cast<float>(opacity);
		if (materialDesc.baseColor.w < 1.0f)
		{
			materialDesc.alphaMode = AlphaMode::Blend;
		}
	}

	aiColor4D emissiveColor;
	if (aiGetMaterialColor(&material, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor) == AI_SUCCESS)
	{
		materialDesc.emissiveColor = DirectX::XMFLOAT3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
	}

	ai_real metallic = 0.0f;
	if (material.Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
	{
		materialDesc.metallic = static_cast<float>(metallic);
	}

	ai_real roughness = 0.5f;
	if (material.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
	{
		materialDesc.roughness = static_cast<float>(roughness);
	}
}

void FbxMaterialImporter::ApplyTextureMappings(
    const aiMaterial& material,
    MaterialHandle materialHandle,
    const std::filesystem::path& sourceDirectory,
    MaterialDesc& materialDesc,
    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
    SceneImportResult& result)
{
	AssignTextureByType(material, materialHandle, sourceDirectory, TextureGroup::Diffuse, materialDesc, outTextureSources, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, TextureGroup::NormalMap, materialDesc, outTextureSources, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, TextureGroup::Roughness, materialDesc, outTextureSources, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, TextureGroup::Metallic, materialDesc, outTextureSources, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, TextureGroup::AmbientOcclusion, materialDesc, outTextureSources, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, TextureGroup::Emissive, materialDesc, outTextureSources, result);
}

void FbxMaterialImporter::AssignTextureByType(
    const aiMaterial& material,
    MaterialHandle materialHandle,
    const std::filesystem::path& sourceDirectory,
    TextureGroup textureGroup,
    MaterialDesc& materialDesc,
    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
    SceneImportResult& result)
{
	switch (textureGroup)
	{
		case TextureGroup::Diffuse:
			SetTextureSource(
			    materialDesc,
			    outTextureSources,
			    textureGroup,
			    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_BASE_COLOR, "base-color", result));

			if (!materialDesc.albedoTexture)
			{
				SetTextureSource(
				    materialDesc,
				    outTextureSources,
				    textureGroup,
				    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_DIFFUSE, "diffuse", result));
			}
			break;

		case TextureGroup::NormalMap:
			SetTextureSource(
			    materialDesc,
			    outTextureSources,
			    textureGroup,
			    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_NORMALS, "normal", result));

			if (!materialDesc.normalTexture)
			{
				SetTextureSource(
				    materialDesc,
				    outTextureSources,
				    textureGroup,
				    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_HEIGHT, "height", result));
			}
			break;

		case TextureGroup::Roughness:
		{
			const std::optional<std::filesystem::path> roughnessTexture =
			    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_DIFFUSE_ROUGHNESS, "roughness", result);
			SetTextureSource(materialDesc, outTextureSources, textureGroup, roughnessTexture, TextureChannelMask::Red);
			if (roughnessTexture)
			{
				materialDesc.roughness = 1.0f;
			}
			break;
		}

		case TextureGroup::Metallic:
		{
			std::optional<std::filesystem::path> metallicTexture =
			    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_METALNESS, "metallic", result);
			if (!metallicTexture)
			{
				metallicTexture = ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_SPECULAR, "specular", result);
			}

			SetTextureSource(materialDesc, outTextureSources, textureGroup, metallicTexture, TextureChannelMask::Red);
			if (metallicTexture)
			{
				materialDesc.metallic = 1.0f;
			}
			break;
		}

		case TextureGroup::AmbientOcclusion:
		{
			std::optional<std::filesystem::path> occlusionTexture =
			    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_AMBIENT_OCCLUSION, "occlusion", result);
			if (!occlusionTexture)
			{
				occlusionTexture =
				    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_SPECULAR, "specular", result);
			}

			SetTextureSource(materialDesc, outTextureSources, textureGroup, occlusionTexture, TextureChannelMask::Red);
			break;
		}

		case TextureGroup::Emissive:
			SetTextureSource(
			    materialDesc,
			    outTextureSources,
			    textureGroup,
			    ResolveTexturePath(material, materialHandle, sourceDirectory, aiTextureType_EMISSIVE, "emissive", result));
			break;

		case TextureGroup::SubsurfaceColor:
		case TextureGroup::SubsurfaceStrength:
		case TextureGroup::Default:
		case TextureGroup::HdrColor:
			break;
	}
}

void FbxMaterialImporter::SetTextureSource(
    MaterialDesc& materialDesc,
    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
    TextureGroup textureGroup,
    const std::optional<std::filesystem::path>& texturePath,
	TextureChannelMask channelMask)
{
	if (!texturePath)
	{
		return;
	}

	materialDesc.SetTexturePath(textureGroup, texturePath);
	outTextureSources.push_back({textureGroup, *texturePath, channelMask});
}

std::optional<std::filesystem::path> FbxMaterialImporter::ResolveTexturePath(
    const aiMaterial& material,
    MaterialHandle materialHandle,
    const std::filesystem::path& sourceDirectory,
    aiTextureType textureType,
    std::string_view slotName,
    SceneImportResult& result)
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
		        materialHandle.GetIndex(),
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
		        materialHandle.GetIndex(),
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
		        materialHandle.GetIndex(),
		        slotName,
		        texturePathString));
		return std::nullopt;
	}

	return NormalizeTexturePath(*resolvedTexturePath, materialHandle, slotName);
}

std::optional<std::filesystem::path> FbxMaterialImporter::NormalizeTexturePath(
    std::filesystem::path texturePath,
    MaterialHandle materialHandle,
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
		        materialHandle.GetIndex(),
		        slotName,
		        texturePath.string()));
		return std::nullopt;
	}

	return normalizedTexturePath;
}