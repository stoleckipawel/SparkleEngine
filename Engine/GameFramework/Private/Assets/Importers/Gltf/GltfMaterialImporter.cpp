#include "PCH.h"

#include "Assets/Importers/Gltf/GltfMaterialImporter.h"

#include "Core/Public/Paths/PathUtils.h"

#include <DirectXMath.h>
#include <cgltf.h>

#include <format>

using namespace DirectX;

void GltfMaterialImporter::ImportMaterials(const cgltf_data* data, const std::filesystem::path& sourceDirectory, SceneImportResult& result)
{
	for (cgltf_size materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
	{
		result.materials.push_back(
		    ExtractMaterial(data->materials[materialIndex], static_cast<unsigned int>(materialIndex), sourceDirectory, result));
	}
}

MaterialDesc GltfMaterialImporter::ExtractMaterial(
	const cgltf_material& material,
	unsigned int materialIndex,
	const std::filesystem::path& sourceDirectory,
	SceneImportResult& result)
{
	const MaterialHandle materialHandle(materialIndex);
	MaterialDesc materialDesc;
	CollectMaterialWarnings(material, materialHandle, result);
	ApplyMaterialProperties(material, materialDesc);
	ApplyTextureMappings(material, materialHandle, sourceDirectory, materialDesc, result);
	return materialDesc;
}

std::optional<std::filesystem::path> GltfMaterialImporter::ResolveTexturePath(
	const cgltf_texture_view& textureView,
	MaterialHandle materialHandle,
	const std::filesystem::path& sourceDirectory,
	std::string_view slotName,
	SceneImportResult& result)
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

		return NormalizeTexturePath(
		    Engine::Paths::ResolveRelativePath(sourceDirectory, std::filesystem::path(texturePathString)),
		    materialHandle,
		    slotName);
	}

	if (texture.image && texture.image->buffer_view)
	{
		LOG_WARNING(std::format(
		    "GltfImporter: Material handle {} uses an embedded {} texture which is not supported yet",
		    materialHandle.GetIndex(),
		    slotName));
		return std::nullopt;
	}

	if (texture.has_basisu || texture.has_webp)
	{
		LOG_WARNING(std::format(
		    "GltfImporter: Material handle {} uses {} texture sources that are not supported by the runtime importer yet",
		    materialHandle.GetIndex(),
		    slotName));
	}

	return std::nullopt;
}

void GltfMaterialImporter::CollectMaterialWarnings(const cgltf_material& material, MaterialHandle materialHandle, SceneImportResult& result)
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
		LOG_WARNING(std::format(
		    "GltfImporter: Material handle {} uses unsupported glTF material features [{}] and will be approximated with Sparkle PBR defaults",
		    materialHandle.GetIndex(),
		    unsupportedFeatures));
	}
}

void GltfMaterialImporter::ApplyMaterialProperties(const cgltf_material& material, MaterialDesc& materialDesc)
{
	materialDesc.emissiveColor = XMFLOAT3(material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]);
	materialDesc.alphaCutoff = material.alpha_cutoff;

	switch (material.alpha_mode)
	{
		case cgltf_alpha_mode_mask:
			materialDesc.alphaMode = AlphaMode::Mask;
			break;
		case cgltf_alpha_mode_blend:
			materialDesc.alphaMode = AlphaMode::Blend;
			break;
		case cgltf_alpha_mode_opaque:
		default:
			materialDesc.alphaMode = AlphaMode::Opaque;
			break;
	}

	if (material.has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;
		materialDesc.baseColor = XMFLOAT4(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]);
		materialDesc.metallic = pbr.metallic_factor;
		materialDesc.roughness = pbr.roughness_factor;
	}
	else if (material.has_pbr_specular_glossiness)
	{
		const cgltf_pbr_specular_glossiness& specGloss = material.pbr_specular_glossiness;
		materialDesc.baseColor = XMFLOAT4(
		    specGloss.diffuse_factor[0],
		    specGloss.diffuse_factor[1],
		    specGloss.diffuse_factor[2],
		    specGloss.diffuse_factor[3]);
		materialDesc.metallic = 0.0f;
		materialDesc.roughness = 1.0f - specGloss.glossiness_factor;
	}
}

void GltfMaterialImporter::ApplyTextureMappings(
	const cgltf_material& material,
	MaterialHandle materialHandle,
	const std::filesystem::path& sourceDirectory,
	MaterialDesc& materialDesc,
	SceneImportResult& result)
{
	AssignTextureByType(material, materialHandle, sourceDirectory, MaterialTextureType::Albedo, materialDesc, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, MaterialTextureType::MetallicRoughness, materialDesc, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, MaterialTextureType::Normal, materialDesc, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, MaterialTextureType::Occlusion, materialDesc, result);
	AssignTextureByType(material, materialHandle, sourceDirectory, MaterialTextureType::Emissive, materialDesc, result);
}

void GltfMaterialImporter::AssignTextureByType(
	const cgltf_material& material,
	MaterialHandle materialHandle,
	const std::filesystem::path& sourceDirectory,
	MaterialTextureType textureType,
	MaterialDesc& materialDesc,
	SceneImportResult& result)
{
	switch (textureType)
	{
		case MaterialTextureType::Albedo:
			if (material.has_pbr_metallic_roughness)
			{
				materialDesc.SetTexturePath(
				    textureType,
				    ResolveTexturePath(
				        material.pbr_metallic_roughness.base_color_texture,
				        materialHandle,
				        sourceDirectory,
				        "base-color",
				        result));
			}
			else if (material.has_pbr_specular_glossiness)
			{
				materialDesc.SetTexturePath(
				    textureType,
				    ResolveTexturePath(
				        material.pbr_specular_glossiness.diffuse_texture,
				        materialHandle,
				        sourceDirectory,
				        "diffuse",
				        result));
			}
			break;

		case MaterialTextureType::MetallicRoughness:
			if (material.has_pbr_metallic_roughness)
			{
				materialDesc.SetTexturePath(
				    textureType,
				    ResolveTexturePath(
				        material.pbr_metallic_roughness.metallic_roughness_texture,
				        materialHandle,
				        sourceDirectory,
				        "metallic-roughness",
				        result));
			}
			break;

		case MaterialTextureType::Normal:
			materialDesc.SetTexturePath(
			    textureType,
			    ResolveTexturePath(material.normal_texture, materialHandle, sourceDirectory, "normal", result));
			break;

		case MaterialTextureType::Occlusion:
			materialDesc.SetTexturePath(
			    textureType,
			    ResolveTexturePath(material.occlusion_texture, materialHandle, sourceDirectory, "occlusion", result));
			break;

		case MaterialTextureType::Emissive:
			materialDesc.SetTexturePath(
			    textureType,
			    ResolveTexturePath(material.emissive_texture, materialHandle, sourceDirectory, "emissive", result));
			break;
	}
}

std::optional<std::filesystem::path> GltfMaterialImporter::NormalizeTexturePath(
	std::filesystem::path texturePath,
	MaterialHandle materialHandle,
	std::string_view slotName)
{
	const std::filesystem::path normalizedTexturePath = Engine::Paths::Normalize(texturePath);
	if (normalizedTexturePath.empty())
	{
		LOG_WARNING(std::format(
		    "GltfImporter: Material handle {} has an invalid {} texture path '{}' and it will be ignored",
		    materialHandle.GetIndex(),
		    slotName,
		    texturePath.string()));
		return std::nullopt;
	}

	return normalizedTexturePath;
}