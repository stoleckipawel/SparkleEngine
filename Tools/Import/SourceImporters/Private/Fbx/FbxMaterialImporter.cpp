#include "PCH.h"

#include "Fbx/FbxMaterialImporter.h"

#include "Core/Public/Diagnostics/Error.h"
#include "SourceTexturePathResolver.h"

#include <array>
#include <format>

struct FbxMaterialTextureMapping final
{
	TextureGroup Group = TextureGroup::Default;
	aiTextureType PreferredType = aiTextureType_NONE;
	std::string_view PreferredName;
	aiTextureType AlternateType = aiTextureType_NONE;
	std::string_view AlternateName;
	TextureChannelMask ChannelMask = TextureChannelMask::Rgba;
};

static constexpr std::array<FbxMaterialTextureMapping, 6> g_fbxMaterialTextureMappings = {
    FbxMaterialTextureMapping{
        .Group = TextureGroup::Diffuse,
        .PreferredType = aiTextureType_BASE_COLOR,
        .PreferredName = "base-color",
        .AlternateType = aiTextureType_DIFFUSE,
        .AlternateName = "diffuse"},
    FbxMaterialTextureMapping{
        .Group = TextureGroup::NormalMap,
        .PreferredType = aiTextureType_NORMALS,
        .PreferredName = "normal",
        .AlternateType = aiTextureType_HEIGHT,
        .AlternateName = "height"},
    FbxMaterialTextureMapping{
        .Group = TextureGroup::Roughness,
        .PreferredType = aiTextureType_DIFFUSE_ROUGHNESS,
        .PreferredName = "roughness",
        .ChannelMask = TextureChannelMask::Red},
    FbxMaterialTextureMapping{
        .Group = TextureGroup::Metallic,
        .PreferredType = aiTextureType_METALNESS,
        .PreferredName = "metallic",
        .ChannelMask = TextureChannelMask::Red},
    FbxMaterialTextureMapping{
        .Group = TextureGroup::AmbientOcclusion,
        .PreferredType = aiTextureType_AMBIENT_OCCLUSION,
        .PreferredName = "occlusion",
        .AlternateType = aiTextureType_LIGHTMAP,
        .AlternateName = "light-map",
        .ChannelMask = TextureChannelMask::Red},
    FbxMaterialTextureMapping{
        .Group = TextureGroup::Emissive,
        .PreferredType = aiTextureType_EMISSION_COLOR,
        .PreferredName = "emission-color",
        .AlternateType = aiTextureType_EMISSIVE,
        .AlternateName = "emissive"}};

class FbxMaterialTextureReferenceReader final
{
  public:
	static std::optional<std::string> Read(
	    const aiMaterial& material,
	    ImportedMaterialIndex materialIndex,
	    aiTextureType textureType,
	    std::string_view slotName)
	{
		const unsigned int textureCount = material.GetTextureCount(textureType);
		if (textureCount == 0)
		{
			return std::nullopt;
		}
		if (textureCount > 1)
		{
			throw Diagnostics::Error(std::format("FBX material {} has multiple {} textures.", materialIndex, slotName));
		}

		aiString texturePath;
		aiTextureMapping mapping = aiTextureMapping_UV;
		unsigned int uvIndex = 0;
		ai_real blend = 1.0f;
		aiTextureOp operation = aiTextureOp_Multiply;
		if (material.GetTexture(textureType, 0, &texturePath, &mapping, &uvIndex, &blend, &operation) != AI_SUCCESS)
		{
			throw Diagnostics::Error(std::format("Cannot read the {} texture reference for FBX material {}.", slotName, materialIndex));
		}
		if (mapping != aiTextureMapping_UV || uvIndex != 0 || blend != 1.0f || operation != aiTextureOp_Multiply)
		{
			throw Diagnostics::Error(std::format("FBX material {} uses an unsupported {} texture mapping.", materialIndex, slotName));
		}

		std::string authoredPath = texturePath.C_Str();
		if (authoredPath.empty())
		{
			throw Diagnostics::Error(std::format("FBX material {} has an empty {} texture path.", materialIndex, slotName));
		}
		return authoredPath;
	}
};

void FbxMaterialImporter::ImportMaterials(
    const aiScene& scene,
    const std::filesystem::path& sourceDirectory,
    std::span<const std::filesystem::path> embeddedTexturePaths,
    SourceImportOutput& output)
{
	const TextureResolutionContext textureContext{scene, sourceDirectory, embeddedTexturePaths};
	for (unsigned int materialIndex = 0; materialIndex < scene.mNumMaterials; ++materialIndex)
	{
		if (scene.mMaterials[materialIndex] == nullptr)
		{
			throw Diagnostics::Error(std::format("FBX material {} is null.", materialIndex));
		}

		ImportedMaterial importedMaterial = ExtractMaterial(*scene.mMaterials[materialIndex], materialIndex, textureContext);
		output.scene.materials.push_back(std::move(importedMaterial));
	}
}

ImportedMaterial FbxMaterialImporter::ExtractMaterial(
    const aiMaterial& material,
    ImportedMaterialIndex materialIndex,
    const TextureResolutionContext& textureContext)
{
	ImportedMaterial importedMaterial;
	ValidateShadingModel(material, materialIndex);
	ApplyMaterialProperties(material, importedMaterial);
	ApplyTextureMappings(material, materialIndex, textureContext, importedMaterial);
	return importedMaterial;
}

void FbxMaterialImporter::ValidateShadingModel(const aiMaterial& material, ImportedMaterialIndex materialIndex)
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

	throw Diagnostics::Error(std::format("FBX material {} uses unsupported shading model {}.", materialIndex, shadingModel));
}

void FbxMaterialImporter::ApplyMaterialProperties(
    const aiMaterial& material,
    ImportedMaterial& importedMaterial)
{
	aiString name;
	if (material.Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
	{
		importedMaterial.name = name.C_Str();
	}

	ApplyMaterialColors(material, importedMaterial);
	ApplyMaterialFactors(material, importedMaterial);
}

void FbxMaterialImporter::ApplyMaterialColors(
    const aiMaterial& material,
    ImportedMaterial& importedMaterial)
{
	aiColor4D baseColor;
	if (aiGetMaterialColor(&material, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS ||
	    aiGetMaterialColor(&material, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS)
	{
		importedMaterial.baseColor = DirectX::XMFLOAT4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
	}

	aiColor4D emissiveColor;
	if (aiGetMaterialColor(&material, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor) == AI_SUCCESS)
	{
		importedMaterial.emissiveColor = DirectX::XMFLOAT3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
	}
}

void FbxMaterialImporter::ApplyMaterialFactors(
    const aiMaterial& material,
    ImportedMaterial& importedMaterial)
{
	ai_real opacity = 1.0f;
	if (material.Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
	{
		importedMaterial.baseColor.w = static_cast<float>(opacity);
		if (importedMaterial.baseColor.w < 1.0f)
		{
			importedMaterial.alphaMode = ImportedAlphaMode::Blend;
		}
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

	int twoSided = 0;
	if (material.Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
	{
		importedMaterial.doubleSided = twoSided != 0;
	}
}

void FbxMaterialImporter::ApplyTextureMappings(
    const aiMaterial& material,
    ImportedMaterialIndex materialIndex,
    const TextureResolutionContext& textureContext,
    ImportedMaterial& importedMaterial)
{
	ValidateTextureMappings(material, materialIndex);

	for (const FbxMaterialTextureMapping& mapping : g_fbxMaterialTextureMappings)
	{
		const std::optional<std::filesystem::path> texturePath =
		    mapping.AlternateType != aiTextureType_NONE
		        ? ResolvePreferredTexturePath(
		              material,
		              materialIndex,
		              textureContext,
		              mapping.PreferredType,
		              mapping.PreferredName,
		              mapping.AlternateType,
		              mapping.AlternateName)
		        : ResolveTexturePath(material, materialIndex, textureContext, mapping.PreferredType, mapping.PreferredName);
		SetTextureSource(importedMaterial, mapping.Group, texturePath, mapping.ChannelMask);
		if (texturePath && mapping.Group == TextureGroup::Roughness)
		{
			importedMaterial.roughness = 1.0f;
		}
		if (texturePath && mapping.Group == TextureGroup::Metallic)
		{
			importedMaterial.metallic = 1.0f;
		}
	}
}

std::optional<std::filesystem::path> FbxMaterialImporter::ResolvePreferredTexturePath(
    const aiMaterial& material,
    ImportedMaterialIndex materialIndex,
    const TextureResolutionContext& textureContext,
    aiTextureType preferredType,
    std::string_view preferredSlotName,
    aiTextureType alternateType,
    std::string_view alternateSlotName)
{
	const bool hasPreferred = material.GetTextureCount(preferredType) > 0;
	const bool hasAlternate = material.GetTextureCount(alternateType) > 0;
	if (hasPreferred && hasAlternate)
	{
		const std::optional<std::filesystem::path> preferredPath =
		    ResolveTexturePath(material, materialIndex, textureContext, preferredType, preferredSlotName);
		const std::optional<std::filesystem::path> alternatePath =
		    ResolveTexturePath(material, materialIndex, textureContext, alternateType, alternateSlotName);
		if (!preferredPath || !alternatePath || *preferredPath != *alternatePath)
		{
			throw Diagnostics::Error(std::format(
			    "FBX material {} assigns conflicting {} and {} textures.",
			    materialIndex,
			    preferredSlotName,
			    alternateSlotName));
		}
		return preferredPath;
	}
	if (hasPreferred)
	{
		return ResolveTexturePath(material, materialIndex, textureContext, preferredType, preferredSlotName);
	}
	return hasAlternate ? ResolveTexturePath(material, materialIndex, textureContext, alternateType, alternateSlotName)
	                    : std::nullopt;
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
    const TextureResolutionContext& textureContext,
    aiTextureType textureType,
    std::string_view slotName)
{
	const std::optional<std::string> texturePath =
	    FbxMaterialTextureReferenceReader::Read(material, materialIndex, textureType, slotName);
	if (!texturePath)
	{
		return std::nullopt;
	}

	const auto [embeddedTexture, embeddedTextureIndex] = textureContext.scene.GetEmbeddedTextureAndIndex(texturePath->c_str());
	if (embeddedTexture != nullptr)
	{
		if (embeddedTextureIndex < 0 || static_cast<std::size_t>(embeddedTextureIndex) >= textureContext.embeddedTexturePaths.size() ||
		    textureContext.embeddedTexturePaths[static_cast<std::size_t>(embeddedTextureIndex)].empty())
		{
			throw Diagnostics::Error(
			    std::format("FBX material {} has unresolved embedded {} texture '{}'.", materialIndex, slotName, *texturePath));
		}

		return textureContext.embeddedTexturePaths[static_cast<std::size_t>(embeddedTextureIndex)];
	}

	if (texturePath->front() == '*')
	{
		throw Diagnostics::Error(
		    std::format("FBX material {} refers to missing embedded {} texture '{}'.", materialIndex, slotName, *texturePath));
	}

	return ResolveExternalTexturePath(*texturePath, textureContext);
}

std::optional<std::filesystem::path> FbxMaterialImporter::ResolveExternalTexturePath(
    std::string_view texturePath,
    const TextureResolutionContext& textureContext)
{
	return SourceTexturePathResolver::ResolveExistingFile(textureContext.sourceDirectory, texturePath);
}

void FbxMaterialImporter::ValidateTextureMappings(const aiMaterial& material, ImportedMaterialIndex materialIndex)
{
	for (int textureTypeValue = static_cast<int>(aiTextureType_DIFFUSE); textureTypeValue <= static_cast<int>(AI_TEXTURE_TYPE_MAX);
	     ++textureTypeValue)
	{
		const aiTextureType textureType = static_cast<aiTextureType>(textureTypeValue);
		switch (textureType)
		{
			case aiTextureType_DIFFUSE:
			case aiTextureType_EMISSIVE:
			case aiTextureType_HEIGHT:
			case aiTextureType_NORMALS:
			case aiTextureType_LIGHTMAP:
			case aiTextureType_BASE_COLOR:
			case aiTextureType_EMISSION_COLOR:
			case aiTextureType_METALNESS:
			case aiTextureType_DIFFUSE_ROUGHNESS:
			case aiTextureType_AMBIENT_OCCLUSION:
				continue;
			default:
				break;
		}

		if (material.GetTextureCount(textureType) > 0)
		{
			throw Diagnostics::Error(std::format(
			    "FBX material {} uses unsupported texture resource type {}.",
			    materialIndex,
			    aiTextureTypeToString(textureType)));
		}
	}
}
