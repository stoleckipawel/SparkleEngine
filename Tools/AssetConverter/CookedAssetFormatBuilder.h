#pragma once

#include "GameFramework/Public/Assets/CookedAssetFormat.h"
#include "GameFramework/Public/Assets/SceneImportResult.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct CookedSceneAssetDefinition
{
	CookedSceneAssetHeader header{};
	std::vector<CookedSceneMeshEntry> meshEntries;
	std::vector<CookedAssetReferenceEntry> references;
	std::vector<char> stringTable;

	bool IsValid() const noexcept { return header.fileHeader.magic == kCookedSceneAssetMagic; }
};

struct CookedMeshAssetDefinition
{
	CookedMeshAssetHeader header{};
	std::vector<CookedMeshEntry> meshTable;
	std::vector<VertexData> vertexBlob;
	std::vector<std::uint32_t> indexBlob;

	bool IsValid() const noexcept { return header.fileHeader.magic == kCookedMeshAssetMagic; }
};

struct CookedMaterialAssetDefinition
{
	CookedMaterialAssetHeader header{};
	std::vector<CookedMaterialEntry> materials;
	std::vector<char> stringTable;

	bool IsValid() const noexcept { return header.fileHeader.magic == kCookedMaterialAssetMagic; }
};

struct CookedTextureManifestDefinition
{
	CookedTextureManifestHeader header{};
	std::vector<CookedTextureEntry> textures;
	std::vector<char> stringTable;

	bool IsValid() const noexcept { return header.fileHeader.magic == kCookedTextureManifestMagic; }
};

struct CookedAssetPackageDefinition
{
	CookedSceneAssetDefinition sceneAsset;
	CookedMeshAssetDefinition meshAsset;
	CookedMaterialAssetDefinition materialAsset;
	CookedTextureManifestDefinition textureManifest;

	bool IsValid() const noexcept
	{
		return sceneAsset.IsValid() &&
		       meshAsset.IsValid() &&
		       materialAsset.IsValid() &&
		       textureManifest.IsValid();
	}

	std::size_t GetMeshCount() const noexcept { return meshAsset.meshTable.size(); }
	std::size_t GetMaterialCount() const noexcept { return materialAsset.materials.size(); }
	std::size_t GetTextureCount() const noexcept { return textureManifest.textures.size(); }
};

struct CookedAssetBuildOptions
{
	std::string assetStem;
	std::filesystem::path meshAssetRelativePath;
	std::filesystem::path materialAssetRelativePath;
	std::filesystem::path textureManifestRelativePath;
	std::filesystem::path cookedTextureDirectory = "textures";
};

class CookedAssetFormatBuilder final
{
  public:
	static CookedAssetPackageDefinition BuildFromSceneImportResult(const SceneImportResult& result);
	static CookedAssetPackageDefinition BuildFromSceneImportResult(
	    const SceneImportResult& result,
	    const CookedAssetBuildOptions& options);

	CookedAssetFormatBuilder() = delete;
	~CookedAssetFormatBuilder() = delete;

  private:
	struct TextureRecord
	{
		std::filesystem::path sourcePath;
		CookedTextureUsageFlags usageFlags = CookedTextureUsageFlags::None;
	};

	static CookedAssetBuildOptions ResolveBuildOptions(
	    const SceneImportResult& result,
	    const CookedAssetBuildOptions* options);
	static CookedAssetPackageDefinition BuildPackage(
	    const SceneImportResult& result,
	    const CookedAssetBuildOptions& options);
	static void BuildMeshAssets(
	    const SceneImportResult& result,
	    const CookedAssetBuildOptions& options,
	    CookedMeshAssetDefinition& meshAsset,
	    CookedSceneAssetDefinition& sceneAsset);
	static void BuildTextureManifest(
	    const SceneImportResult& result,
	    const CookedAssetBuildOptions& options,
	    std::vector<TextureRecord>& textureRecords,
	    CookedTextureManifestDefinition& textureManifest);
	static void BuildMaterialAssets(
	    const SceneImportResult& result,
	    const std::vector<TextureRecord>& textureRecords,
	    CookedMaterialAssetDefinition& materialAsset);
	static void BuildSceneReferences(
	    const CookedAssetBuildOptions& options,
	    CookedSceneAssetDefinition& sceneAsset);
	static void FinalizeHeaders(CookedAssetPackageDefinition& packageDefinition) noexcept;

	static std::string ResolveAssetStem(const SceneImportResult& result, const CookedAssetBuildOptions* options);
	static std::filesystem::path ResolveRelativePath(
	    std::string_view assetStem,
	    const std::filesystem::path& configuredPath,
	    std::string_view extension);
	static std::string BuildSyntheticMeshLabel(std::string_view assetStem, std::size_t meshIndex);
	static std::uint64_t HashMeshLabel(std::string_view meshLabel) noexcept;
	static CookedStringRef AppendString(std::vector<char>& stringTable, std::string_view value);
	static CookedStringRef AppendPathString(std::vector<char>& stringTable, const std::filesystem::path& path);
	static std::int32_t AddOrUpdateTextureRecord(
	    std::vector<TextureRecord>& textureRecords,
	    const std::filesystem::path& texturePath,
	    CookedTextureUsageFlags usageFlags);
	static std::int32_t ResolveTextureIndex(
	    const std::vector<TextureRecord>& textureRecords,
	    const std::optional<std::filesystem::path>& texturePath);
	static CookedTextureFormatHint DeriveTextureFormatHint(CookedTextureUsageFlags usageFlags) noexcept;
	static std::filesystem::path BuildCookedTextureReference(
	    const std::filesystem::path& cookedTextureDirectory,
	    const std::filesystem::path& sourceTexturePath,
	    CookedTextureUsageFlags usageFlags);
	static std::string SanitizeIdentifier(std::string_view identifier);
};