#pragma once

#include "SourceImportResult.h"

#include <assimp/material.h>
#include <assimp/scene.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

class FbxMaterialImporter final
{
  public:
	static void ImportMaterials(const aiScene& scene, const std::filesystem::path& sourceDirectory, SourceImportResult& result);

  private:
	static ImportedMaterial ExtractMaterial(
	    const aiMaterial& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    SourceImportResult& result);
	static void CollectMaterialWarnings(const aiMaterial& material, ImportedMaterialIndex materialIndex, SourceImportResult& result);
	static void ApplyMaterialProperties(const aiMaterial& material, ImportedMaterial& importedMaterial);
	static void ApplyTextureMappings(
	    const aiMaterial& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    ImportedMaterial& importedMaterial,
	    SourceImportResult& result);
	static void AssignTextureByType(
	    const aiMaterial& material,
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
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const aiMaterial& material,
	    ImportedMaterialIndex materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    aiTextureType textureType,
	    std::string_view slotName,
	    SourceImportResult& result);
	static std::optional<std::filesystem::path> NormalizeTexturePath(
	    std::filesystem::path texturePath,
	    ImportedMaterialIndex materialIndex,
	    std::string_view slotName);
};


