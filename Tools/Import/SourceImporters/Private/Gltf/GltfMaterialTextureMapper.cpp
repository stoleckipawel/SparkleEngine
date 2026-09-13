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
		const cgltf_texture_view& textureView = material.pbr_metallic_roughness.metallic_roughness_texture;
		const std::optional<std::filesystem::path> texturePath = ResolveTexturePath(
		    textureView,
		    materialIndex,
		    sourceDirectory,
		    "metallic-roughness");
		const TextureCoordinateMapping mapping = BuildTextureMapping(textureView);
		SetTextureSource(importedMaterial, TextureGroup::Roughness, texturePath, TextureChannelMask::Green, mapping);
		SetTextureSource(importedMaterial, TextureGroup::Metallic, texturePath, TextureChannelMask::Blue, mapping);
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
				const cgltf_texture_view& textureView = material.pbr_metallic_roughness.base_color_texture;
				SetTextureSource(
				    importedMaterial,
				    textureGroup,
				    ResolveTexturePath(textureView, materialIndex, sourceDirectory, "base-color"),
				    TextureChannelMask::Rgba,
				    BuildTextureMapping(textureView));
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
		{
			const cgltf_texture_view& textureView = material.normal_texture;
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(textureView, materialIndex, sourceDirectory, "normal"),
			    TextureChannelMask::Rgba,
			    BuildTextureMapping(textureView));
			break;
		}

		case TextureGroup::AmbientOcclusion:
		{
			const cgltf_texture_view& textureView = material.occlusion_texture;
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(textureView, materialIndex, sourceDirectory, "occlusion"),
			    TextureChannelMask::Red,
			    BuildTextureMapping(textureView));
			break;
		}

		case TextureGroup::Emissive:
		{
			const cgltf_texture_view& textureView = material.emissive_texture;
			SetTextureSource(
			    importedMaterial,
			    textureGroup,
			    ResolveTexturePath(textureView, materialIndex, sourceDirectory, "emissive"),
			    TextureChannelMask::Rgba,
			    BuildTextureMapping(textureView));
			break;
		}
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
	const std::uint32_t texCoord = textureView.has_transform && textureView.transform.has_texcoord
	    ? static_cast<std::uint32_t>(textureView.transform.texcoord)
	    : static_cast<std::uint32_t>(textureView.texcoord);
	if (texCoord != 0u)
	{
		throw Diagnostics::Error(
		    std::format("glTF material {} uses an unsupported {} texture coordinate mapping.", materialIndex, slotName));
	}
	const cgltf_texture& texture = *textureView.texture;
	if (texture.has_basisu || texture.has_webp)
	{
		throw Diagnostics::Error(std::format("glTF material {} uses an unsupported {} texture encoding.", materialIndex, slotName));
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

TextureCoordinateMapping GltfMaterialTextureMapper::BuildTextureMapping(const cgltf_texture_view& textureView)
{
	TextureCoordinateMapping mapping;
	mapping.Strength = textureView.scale;
	if (textureView.has_transform)
	{
		mapping.Offset = {textureView.transform.offset[0], textureView.transform.offset[1]};
		mapping.Scale = {textureView.transform.scale[0], textureView.transform.scale[1]};
		mapping.Rotation = textureView.transform.rotation;
		mapping.TexCoord = textureView.transform.has_texcoord ? static_cast<std::uint32_t>(textureView.transform.texcoord)
		                                                      : static_cast<std::uint32_t>(textureView.texcoord);
	}
	else
	{
		mapping.TexCoord = static_cast<std::uint32_t>(textureView.texcoord);
	}

	if (textureView.texture && textureView.texture->sampler)
	{
		auto addressMode = [](cgltf_wrap_mode wrap)
		{
			switch (wrap)
			{
				case cgltf_wrap_mode_clamp_to_edge:
					return TextureAddressMode::ClampToEdge;
				case cgltf_wrap_mode_mirrored_repeat:
					return TextureAddressMode::MirroredRepeat;
				case cgltf_wrap_mode_repeat:
				default:
					return TextureAddressMode::Repeat;
			}
		};
		mapping.AddressU = addressMode(textureView.texture->sampler->wrap_s);
		mapping.AddressV = addressMode(textureView.texture->sampler->wrap_t);
	}
	return mapping;
}

void GltfMaterialTextureMapper::SetTextureSource(
    ImportedMaterial& importedMaterial,
    TextureGroup textureGroup,
    const std::optional<std::filesystem::path>& texturePath,
    TextureChannelMask channelMask,
    TextureCoordinateMapping mapping)
{
	if (!texturePath)
	{
		return;
	}

	importedMaterial.textureSources.push_back({textureGroup, *texturePath, channelMask, mapping});
}
