#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <assimp/material.h>
#include <assimp/scene.h>

#include <filesystem>
#include <optional>
#include <string_view>

class FbxMaterialImporter final
{
  public:
	static void ImportMaterials(const aiScene& scene, const std::filesystem::path& sourceDirectory, SceneImportResult& result);

  private:
	static MaterialDesc ExtractMaterial(
	    const aiMaterial& material,
	    unsigned int materialIndex,
	    const std::filesystem::path& sourceDirectory,
	    SceneImportResult& result);
	static void CollectMaterialWarnings(const aiMaterial& material, MaterialHandle materialHandle, SceneImportResult& result);
	static void ApplyMaterialProperties(const aiMaterial& material, MaterialDesc& materialDesc);
	static void ApplyTextureMappings(
	    const aiMaterial& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    MaterialDesc& materialDesc,
	    SceneImportResult& result);
	static void AssignTextureByType(
	    const aiMaterial& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    MaterialTextureType textureType,
	    MaterialDesc& materialDesc,
	    SceneImportResult& result);
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const aiMaterial& material,
	    MaterialHandle materialHandle,
	    const std::filesystem::path& sourceDirectory,
	    aiTextureType textureType,
	    std::string_view slotName,
	    SceneImportResult& result);
};