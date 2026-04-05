#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <filesystem>
#include <optional>
#include <string_view>

struct cgltf_data;
struct cgltf_material;
struct cgltf_texture_view;

class GltfMaterialImporter final
{
  public:
	static void ImportMaterials(const cgltf_data* data, const std::filesystem::path& gltfDirectory, SceneImportResult& result);

  private:
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const cgltf_texture_view& textureView,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& gltfDirectory,
	    std::string_view slotName,
	    SceneImportResult& result);
	static void AssignTextureByType(
	    const cgltf_material& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& gltfDirectory,
	    MaterialTextureType textureType,
	    MaterialDesc& materialDesc,
	    SceneImportResult& result);
	static void AppendUnsupportedMaterialWarnings(const cgltf_material& material, MaterialHandle materialHandle, SceneImportResult& result);
};