#pragma once

#include "SourceImportOutput.h"

#include <assimp/material.h>
#include <assimp/scene.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

class FbxMaterialImporter final
{
public:
	static void ImportMaterials(
	    const aiScene& scene,
	    const std::filesystem::path& sourceDirectory,
	    std::span<const std::filesystem::path> embeddedTexturePaths,
	    SourceImportOutput& output);

private:
	struct TextureResolutionContext
	{
		const aiScene& scene;
		const std::filesystem::path& sourceDirectory;
		std::span<const std::filesystem::path> embeddedTexturePaths;
	};

	static ImportedMaterial ExtractMaterial(
	    const aiMaterial& material,
	    ImportedMaterialIndex materialIndex,
	    const TextureResolutionContext& textureContext);
	static void ValidateShadingModel(const aiMaterial& material, ImportedMaterialIndex materialIndex);
	static void ApplyMaterialProperties(const aiMaterial& material, ImportedMaterial& importedMaterial);
	static void ApplyMaterialColors(const aiMaterial& material, ImportedMaterial& importedMaterial);
	static void ApplyMaterialFactors(const aiMaterial& material, ImportedMaterial& importedMaterial);
	static void ApplyTextureMappings(
	    const aiMaterial& material,
	    ImportedMaterialIndex materialIndex,
	    const TextureResolutionContext& textureContext,
	    ImportedMaterial& importedMaterial);
	static std::optional<std::filesystem::path> ResolvePreferredTexturePath(
	    const aiMaterial& material,
	    ImportedMaterialIndex materialIndex,
	    const TextureResolutionContext& textureContext,
	    aiTextureType preferredType,
	    std::string_view preferredSlotName,
	    aiTextureType alternateType,
	    std::string_view alternateSlotName);
	static void SetTextureSource(
	    ImportedMaterial& importedMaterial,
	    TextureGroup textureGroup,
	    const std::optional<std::filesystem::path>& texturePath,
	    TextureChannelMask channelMask = TextureChannelMask::Rgba);
	static std::optional<std::filesystem::path> ResolveTexturePath(
	    const aiMaterial& material,
	    ImportedMaterialIndex materialIndex,
	    const TextureResolutionContext& textureContext,
	    aiTextureType textureType,
	    std::string_view slotName);
	static std::optional<std::filesystem::path> ResolveExternalTexturePath(
	    std::string_view texturePath,
	    const TextureResolutionContext& textureContext);
	static void ValidateTextureMappings(const aiMaterial& material, ImportedMaterialIndex materialIndex);
};
