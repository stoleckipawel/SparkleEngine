#pragma once

#include "Types/ImportedMaterial.h"
#include "Types/ImportedSceneIndices.h"

#include <filesystem>
#include <optional>
#include <string_view>

struct cgltf_material;
struct cgltf_texture_view;

class GltfMaterialTextureMapper final
{
public:
	static void Apply(
	    const cgltf_material& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    ImportedMaterial& importedMaterial);

private:
	static void AssignPackedMetallicRoughness(
	    const cgltf_material& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    ImportedMaterial& importedMaterial);
	static void AssignTextureByType(
	    const cgltf_material& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    TextureGroup textureGroup,
	    ImportedMaterial& importedMaterial);
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const cgltf_texture_view& textureView,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    std::string_view slotName);
	static void SetTextureSource(
	    ImportedMaterial& importedMaterial,
	    TextureGroup textureGroup,
	    const std::optional<std::filesystem::path>& texturePath,
	    TextureChannelMask channelMask = TextureChannelMask::Rgba);
};
