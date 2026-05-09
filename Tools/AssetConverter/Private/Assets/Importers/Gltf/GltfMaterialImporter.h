#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

struct cgltf_data;
struct cgltf_material;
struct cgltf_texture_view;

class GltfMaterialImporter final
{
  public:
	static void ImportMaterials(const cgltf_data* data, const std::filesystem::path& sourceDirectory, SceneImportResult& result);

  private:
	static MaterialDesc ExtractMaterial(
	    const cgltf_material& material,
	    unsigned int materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
	    SceneImportResult& result);
	static void CollectMaterialWarnings(const cgltf_material& material, MaterialHandle materialHandle, SceneImportResult& result);
	static void ApplyMaterialProperties(const cgltf_material& material, MaterialDesc& materialDesc);
	static void ApplyTextureMappings(
	    const cgltf_material& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    MaterialDesc& materialDesc,
	    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
	    SceneImportResult& result);
	static void AssignPackedMetallicRoughness(
	    const cgltf_material& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    MaterialDesc& materialDesc,
	    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
	    SceneImportResult& result);
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const cgltf_texture_view& textureView,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    std::string_view slotName,
	    SceneImportResult& result);
	static void AssignTextureByType(
	    const cgltf_material& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    TextureGroup textureGroup,
	    MaterialDesc& materialDesc,
	    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
	    SceneImportResult& result);
	static void SetTextureSource(
	    MaterialDesc& materialDesc,
	    std::vector<SceneImportResult::MaterialTextureSource>& outTextureSources,
	    TextureGroup textureGroup,
	    const std::optional<std::filesystem::path>& texturePath,
		    TextureChannelMask channelMask = TextureChannelMask::Rgba);
	static std::optional<std::filesystem::path> NormalizeTexturePath(
	    std::filesystem::path texturePath,
	    MaterialHandle materialHandle,
	    std::string_view slotName);
};