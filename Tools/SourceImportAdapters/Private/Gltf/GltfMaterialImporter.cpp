#include "PCH.h"

#include "Gltf/GltfMaterialImporter.h"

#include "Core/Public/Paths/PathUtils.h"
#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "Diagnostics/SourceImportDiagnosticsRecorder.h"

#include <DirectXMath.h>
#include <cgltf.h>

void GltfMaterialImporter::ImportMaterials(const cgltf_data* data, const std::filesystem::path& sourceDirectory, SourceImportResult& result)
{
	for (cgltf_size materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
	{
		result.scene.materials.push_back(ExtractMaterial(
		    data->materials[materialIndex],
		    static_cast<ImportedMaterialIndex>(materialIndex),
		    sourceDirectory,
		    result));
	}
}

ImportedMaterial GltfMaterialImporter::ExtractMaterial(
    const cgltf_material& material,
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

std::optional<std::filesystem::path> GltfMaterialImporter::ResolveTexturePath(
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

void GltfMaterialImporter::CollectMaterialWarnings(const cgltf_material& material, ImportedMaterialIndex materialIndex, SourceImportResult& result)
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
		GltfImportDiagnosticLog::ReportUnsupportedMaterialFeatures(materialIndex, unsupportedFeatures, result);
	}
}

void GltfMaterialImporter::ApplyMaterialProperties(const cgltf_material& material, ImportedMaterial& importedMaterial)
{
	importedMaterial.emissiveColor = DirectX::XMFLOAT3(material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]);
	importedMaterial.alphaCutoff = material.alpha_cutoff;

	switch (material.alpha_mode)
	{
		case cgltf_alpha_mode_mask:
			importedMaterial.alphaMode = ImportedAlphaMode::Mask;
			break;
		case cgltf_alpha_mode_blend:
			importedMaterial.alphaMode = ImportedAlphaMode::Blend;
			break;
		case cgltf_alpha_mode_opaque:
		default:
			importedMaterial.alphaMode = ImportedAlphaMode::Opaque;
			break;
	}

	if (material.has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;
		importedMaterial.baseColor =
		    DirectX::XMFLOAT4(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]);
		importedMaterial.metallic = pbr.metallic_factor;
		importedMaterial.roughness = pbr.roughness_factor;
	}
	else if (material.has_pbr_specular_glossiness)
	{
		const cgltf_pbr_specular_glossiness& specGloss = material.pbr_specular_glossiness;
		importedMaterial.baseColor =
		    DirectX::XMFLOAT4(
		        specGloss.diffuse_factor[0],
		        specGloss.diffuse_factor[1],
		        specGloss.diffuse_factor[2],
		        specGloss.diffuse_factor[3]);
		importedMaterial.metallic = 0.0f;
		importedMaterial.roughness = 1.0f - specGloss.glossiness_factor;
	}
}

void GltfMaterialImporter::ApplyTextureMappings(
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

void GltfMaterialImporter::AssignPackedMetallicRoughness(
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

void GltfMaterialImporter::AssignTextureByType(
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

void GltfMaterialImporter::SetTextureSource(
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

std::optional<std::filesystem::path> GltfMaterialImporter::NormalizeTexturePath(
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


