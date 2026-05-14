#pragma once

#include "SourceImportResult.h"

#include <assimp/material.h>
#include <assimp/scene.h>

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

class FbxMaterialImporter final
{
  public:
	static void ImportMaterials(const aiScene& scene, const std::filesystem::path& sourceDirectory, SourceImportResult& result);

  private:
	static MaterialDesc ExtractMaterial(
	    const aiMaterial& material,
	    unsigned int materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    std::vector<SourceImportResult::TextureSource>& outTextureSources,
	    SourceImportResult& result);
	static void CollectMaterialWarnings(const aiMaterial& material, MaterialHandle materialHandle, SourceImportResult& result);
	static void ApplyMaterialProperties(const aiMaterial& material, MaterialDesc& materialDesc);
	static void ApplyTextureMappings(
	    const aiMaterial& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    MaterialDesc& materialDesc,
	    std::vector<SourceImportResult::TextureSource>& outTextureSources,
	    SourceImportResult& result);
	static void AssignTextureByType(
	    const aiMaterial& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    TextureGroup textureGroup,
	    MaterialDesc& materialDesc,
	    std::vector<SourceImportResult::TextureSource>& outTextureSources,
	    SourceImportResult& result);
	static void SetTextureSource(
	    MaterialDesc& materialDesc,
	    std::vector<SourceImportResult::TextureSource>& outTextureSources,
	    TextureGroup textureGroup,
	    const std::optional<std::filesystem::path>& texturePath,
		    TextureChannelMask channelMask = TextureChannelMask::Rgba);
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const aiMaterial& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    aiTextureType textureType,
	    std::string_view slotName,
	    SourceImportResult& result);
	static std::optional<std::filesystem::path> NormalizeTexturePath(
	    std::filesystem::path texturePath,
	    MaterialHandle materialHandle,
	    std::string_view slotName);
};


