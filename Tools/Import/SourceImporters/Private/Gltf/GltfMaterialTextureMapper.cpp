#include "PCH.h"

#include "Gltf/GltfMaterialTextureMapper.h"

#include "Core/Public/Paths/PathUtils.h"

#include <cgltf.h>

#include <format>

static const auto g_gltfMaterialTextureMapperLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Gltf");

void GltfMaterialTextureMapper::Apply(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    ImportedMaterial& importedMaterial,
    SourceImportResult& result)
{
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::Diffuse, importedMaterial, result);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::NormalMap, importedMaterial, result);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::AmbientOcclusion, importedMaterial, result);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::Emissive, importedMaterial, result);
	AssignPackedMetallicRoughness(material, materialIndex, sourceDirectory, importedMaterial, result);
}

void GltfMaterialTextureMapper::AssignPackedMetallicRoughness(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    ImportedMaterial& importedMaterial,
    SourceImportResult& result)
{
	if (material.has_pbr_metallic_roughness && material.pbr_metallic_roughness.metallic_roughness_texture.texture)
	{
		const std::optional<std::filesystem::path> texturePath = ResolveTexturePath(
		    material.pbr_metallic_roughness.metallic_roughness_texture,
		    materialIndex,
		    sourceDirectory,
		    "metallic-roughness",
		    result);
		SetTextureSource(importedMaterial, TextureGroup::Roughness, texturePath, TextureChannelMask::Green);
		SetTextureSource(importedMaterial, TextureGroup::Metallic, texturePath, TextureChannelMask::Blue);
	}
}

void GltfMaterialTextureMapper::AssignTextureByType(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    TextureGroup textureGroup,
    ImportedMaterial& importedMaterial,
    SourceImportResult& result)
{
	switch (textureGroup)
	{
		case TextureGroup::Diffuse:
			if (material.has_pbr_metallic_roughness)
			{
				SetTextureSource(
				    importedMaterial,
				    textureGroup,
				    ResolveTexturePath(
				        material.pbr_metallic_roughness.base_color_texture,
				        materialIndex,
				        sourceDirectory,
				        "base-color",
				        result));
			}
			else if (material.has_pbr_specular_glossiness)
			{
				SetTextureSource(
				    importedMaterial,
				    textureGroup,
				    ResolveTexturePath(
				        material.pbr_specular_glossiness.diffuse_texture,
				        materialIndex,
				        sourceDirectory,
				        "diffuse",
				        result));
			}
			break;

		case TextureGroup::Roughness:
		case TextureGroup::Metallic:
		case TextureGroup::SubsurfaceColor:
		case TextureGroup::SubsurfaceStrength:
		case TextureGroup::Default:
		case TextureGroup::HdrColor:
			break;

		case TextureGroup::NormalMap:
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(material.normal_texture, materialIndex, sourceDirectory, "normal", result));
			break;

		case TextureGroup::AmbientOcclusion:
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(material.occlusion_texture, materialIndex, sourceDirectory, "occlusion", result),
			    TextureChannelMask::Red);
			break;

		case TextureGroup::Emissive:
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(material.emissive_texture, materialIndex, sourceDirectory, "emissive", result));
			break;
	}
}

std::optional<std::filesystem::path> GltfMaterialTextureMapper::ResolveTexturePath(
    const cgltf_texture_view& textureView,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    std::string_view slotName,
    SourceImportResult& result)
{
	if (!textureView.texture)
	{
		return std::nullopt;
	}

	const cgltf_texture& texture = *textureView.texture;
	if (texture.image && texture.image->uri)
	{
		const std::string texturePathString = texture.image->uri;
		if (texturePathString.empty())
		{
			return std::nullopt;
		}

		const std::optional<std::filesystem::path> resolvedTexturePath =
		    Paths::ResolveRelativePath(sourceDirectory, std::filesystem::path(texturePathString));
		if (!resolvedTexturePath)
		{
			(void)result;
			SPDLOG_LOGGER_WARN(
			    g_gltfMaterialTextureMapperLogger,
			    "{}",
			    std::format(
			        "GltfImporter: Material handle {} has an invalid {} texture path '{}' and it will be ignored",
			        materialIndex,
			        slotName,
			        texturePathString));
			return std::nullopt;
		}

		return NormalizeTexturePath(*resolvedTexturePath, materialIndex, slotName, result);
	}

	if (texture.image && texture.image->buffer_view)
	{
		SPDLOG_LOGGER_WARN(
		    g_gltfMaterialTextureMapperLogger,
		    "{}",
		    std::format("GltfImporter: Material handle {} uses an embedded {} texture which is not supported yet", materialIndex, slotName));
		return std::nullopt;
	}

	if (texture.has_basisu || texture.has_webp)
	{
		SPDLOG_LOGGER_WARN(
		    g_gltfMaterialTextureMapperLogger,
		    "{}",
		    std::format(
		        "GltfImporter: Material handle {} uses {} texture sources that are not supported by the runtime importer yet",
		        materialIndex,
		        slotName));
	}

	return std::nullopt;
}

std::optional<std::filesystem::path> GltfMaterialTextureMapper::NormalizeTexturePath(
    std::filesystem::path texturePath,
    ImportedMaterialIndex materialIndex,
    std::string_view slotName,
    SourceImportResult& result)
{
	const std::filesystem::path normalizedTexturePath = Paths::Normalize(texturePath);
	if (normalizedTexturePath.empty())
	{
		(void)result;
		SPDLOG_LOGGER_WARN(
		    g_gltfMaterialTextureMapperLogger,
		    "{}",
		    std::format(
		        "GltfImporter: Material handle {} has an invalid {} texture path '{}' and it will be ignored",
		        materialIndex,
		        slotName,
		        texturePath.string()));
		return std::nullopt;
	}

	return normalizedTexturePath;
}

void GltfMaterialTextureMapper::SetTextureSource(
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
