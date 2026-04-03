#pragma once

#include "CookedAssetFormatBuilder.h"
#include "GameFramework/Public/Assets/SceneImportResult.h"

#include <filesystem>
#include <string>

struct CookedAssetWriteResult
{
	bool bSuccess = false;
	std::filesystem::path sceneAssetPath;
	std::string errorMessage;
};

class CookedAssetWriter final
{
  public:
	static CookedAssetWriteResult Write(const SceneImportResult& result, const std::filesystem::path& outputDirectory);

	CookedAssetWriter() = delete;
	~CookedAssetWriter() = delete;

  private:
	static bool WriteSceneAsset(
	    const CookedSceneAssetDefinition& sceneAsset,
	    const std::filesystem::path& outputPath,
	    std::string& errorMessage);
	static bool WriteMeshAsset(
	    const CookedMeshAssetDefinition& meshAsset,
	    const std::filesystem::path& outputPath,
	    std::string& errorMessage);
	static bool WriteMaterialAsset(
	    const CookedMaterialAssetDefinition& materialAsset,
	    const std::filesystem::path& outputPath,
	    std::string& errorMessage);
	static bool WriteTextureManifest(
	    const CookedTextureManifestDefinition& textureManifest,
	    const std::filesystem::path& outputPath,
	    std::string& errorMessage);
	static bool OpenOutputFile(
	    const std::filesystem::path& outputPath,
	    std::ofstream& outputStream,
	    std::string& errorMessage);
	static bool WriteSceneAssetPayload(std::ofstream& outputStream, const CookedSceneAssetDefinition& sceneAsset);
	static bool WriteMeshAssetPayload(std::ofstream& outputStream, const CookedMeshAssetDefinition& meshAsset);
	static bool WriteMaterialAssetPayload(std::ofstream& outputStream, const CookedMaterialAssetDefinition& materialAsset);
	static bool WriteTextureManifestPayload(std::ofstream& outputStream, const CookedTextureManifestDefinition& textureManifest);
	static std::filesystem::path GetReferencePath(
	    const std::vector<char>& stringTable,
	    const CookedStringRef& stringRef,
	    const std::filesystem::path& fallbackPath);
	static std::string_view GetStringView(const std::vector<char>& stringTable, const CookedStringRef& stringRef) noexcept;
	static std::string SanitizeIdentifier(std::string_view identifier);
};
