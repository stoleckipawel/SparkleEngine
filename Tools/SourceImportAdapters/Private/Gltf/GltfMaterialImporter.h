#pragma once

#include "SourceImportResult.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

struct cgltf_data;
struct cgltf_material;
struct cgltf_texture_view;

class GltfMaterialImporter final
{
  public:
	static void ImportMaterials(const cgltf_data* data, const std::filesystem::path& sourceDirectory, SourceImportResult& result);

  private:
	static ImportedMaterial ExtractMaterial(
	    const cgltf_material& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    SourceImportResult& result);
	static void CollectMaterialWarnings(const cgltf_material& material, ImportedMaterialIndex materialIndex, SourceImportResult& result);
	static void ApplyMaterialProperties(const cgltf_material& material, ImportedMaterial& importedMaterial);
	static void ApplyTextureMappings(
	    const cgltf_material& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    ImportedMaterial& importedMaterial,
	    SourceImportResult& result);
	static void AssignPackedMetallicRoughness(
	    const cgltf_material& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    ImportedMaterial& importedMaterial,
	    SourceImportResult& result);
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const cgltf_texture_view& textureView,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    std::string_view slotName,
	    SourceImportResult& result);
	static void AssignTextureByType(
	    const cgltf_material& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    TextureGroup textureGroup,
	    ImportedMaterial& importedMaterial,
	    SourceImportResult& result);
	static void SetTextureSource(
	    ImportedMaterial& importedMaterial,
	    TextureGroup textureGroup,
	    const std::optional<std::filesystem::path>& texturePath,
	    TextureChannelMask channelMask = TextureChannelMask::Rgba);
	static std::optional<std::filesystem::path> NormalizeTexturePath(
	    std::filesystem::path texturePath,
	    ImportedMaterialIndex materialIndex,
	    std::string_view slotName);
};


