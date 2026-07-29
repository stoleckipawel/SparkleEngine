#include "PCH.h"

#include "Gltf/GltfMaterialTextureMapper.h"

#include "Core/Public/Diagnostics/Error.h"
#include "SourceTexturePathResolver.h"

#include <cgltf.h>

#include <format>

void GltfMaterialTextureMapper::Apply(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    ImportedMaterial& importedMaterial)
{
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::Diffuse, importedMaterial);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::NormalMap, importedMaterial);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::AmbientOcclusion, importedMaterial);
	AssignTextureByType(material, materialIndex, sourceDirectory, TextureGroup::Emissive, importedMaterial);
	AssignPackedMetallicRoughness(material, materialIndex, sourceDirectory, importedMaterial);
}

void GltfMaterialTextureMapper::AssignPackedMetallicRoughness(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    ImportedMaterial& importedMaterial)
{
	if (material.has_pbr_metallic_roughness && material.pbr_metallic_roughness.metallic_roughness_texture.texture)
	{
		const std::optional<std::filesystem::path> texturePath = ResolveTexturePath(
		    material.pbr_metallic_roughness.metallic_roughness_texture,
		    materialIndex,
		    sourceDirectory,
		    "metallic-roughness");
		SetTextureSource(importedMaterial, TextureGroup::Roughness, texturePath, TextureChannelMask::Green);
		SetTextureSource(importedMaterial, TextureGroup::Metallic, texturePath, TextureChannelMask::Blue);
	}
}

void GltfMaterialTextureMapper::AssignTextureByType(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    TextureGroup textureGroup,
    ImportedMaterial& importedMaterial)
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
				        "base-color"));
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
			    ResolveTexturePath(material.normal_texture, materialIndex, sourceDirectory, "normal"));
			break;

		case TextureGroup::AmbientOcclusion:
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(material.occlusion_texture, materialIndex, sourceDirectory, "occlusion"),
			    TextureChannelMask::Red);
			break;

		case TextureGroup::Emissive:
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(material.emissive_texture, materialIndex, sourceDirectory, "emissive"));
			break;
	}
}

std::optional<std::filesystem::path> GltfMaterialTextureMapper::ResolveTexturePath(
    const cgltf_texture_view& textureView,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    std::string_view slotName)
{
	if (!textureView.texture)
	{
		return std::nullopt;
	}
	if (textureView.texcoord != 0 || textureView.has_transform || textureView.scale != 1.0f)
	{
		throw Diagnostics::Error(
		    std::format("glTF material {} uses an unsupported {} texture coordinate mapping.", materialIndex, slotName));
	}

	const cgltf_texture& texture = *textureView.texture;
	if (texture.has_basisu || texture.has_webp)
	{
		throw Diagnostics::Error(
		    std::format("glTF material {} uses an unsupported {} texture encoding.", materialIndex, slotName));
	}

	if (texture.image && texture.image->uri)
	{
		const std::string texturePathString = texture.image->uri;
		if (texturePathString.empty())
		{
			throw Diagnostics::Error(std::format("glTF material {} has an empty {} texture URI.", materialIndex, slotName));
		}

		return SourceTexturePathResolver::ResolveExistingFile(sourceDirectory, texturePathString);
	}

	if (texture.image && texture.image->buffer_view)
	{
		throw Diagnostics::Error(std::format("glTF material {} uses an unsupported embedded {} texture.", materialIndex, slotName));
	}

	throw Diagnostics::Error(std::format("glTF material {} has no supported {} texture source.", materialIndex, slotName));
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
