#include "PCH.h"

#include "Gltf/GltfMaterialTextureMapper.h"

#include "Core/Public/Paths/PathUtils.h"
#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"

#include <cgltf.h>

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
	SourceImportDiagnosticsRecorder::RecordReferencedTextureBindings(result);

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
			GltfImportDiagnosticLog::ReportInvalidTexturePath(materialIndex, slotName, texturePathString, result);
			return std::nullopt;
		}

		return NormalizeTexturePath(*resolvedTexturePath, materialIndex, slotName, result);
	}

	if (texture.image && texture.image->buffer_view)
	{
		GltfImportDiagnosticLog::ReportEmbeddedTexture(materialIndex, slotName, result);
		return std::nullopt;
	}

	if (texture.has_basisu || texture.has_webp)
	{
		GltfImportDiagnosticLog::ReportUnsupportedEncodedTextureSources(materialIndex, slotName, result);
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
		GltfImportDiagnosticLog::ReportInvalidTexturePath(materialIndex, slotName, texturePath.string(), result);
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
