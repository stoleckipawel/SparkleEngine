#include "PCH.h"

#include "MeshDiagnosticMetadataCatalog.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

class CookedMeshMetadataReader final
{
  public:

static std::filesystem::path BuildCookedMeshMetadataPath(std::uint64_t meshAssetId)
{
	std::filesystem::path metadataPath = Paths::CookedMeshAsset(meshAssetId);
	metadataPath += ".meta.json";
	return metadataPath;
}

static std::optional<MeshDiagnosticMetadata> LoadCookedMeshMetadata(std::uint64_t meshAssetId)
{
	std::string metadataText;
	std::string readError;
	if (!Files::TryReadAllText(BuildCookedMeshMetadataPath(meshAssetId), metadataText, readError))
	{
		return std::nullopt;
	}

	std::string schema;
	if (!Json::TryReadStringProperty(metadataText, "schema", schema) || schema != "cooked-mesh-metadata-v1")
	{
		return std::nullopt;
	}

	MeshDiagnosticMetadata metadata;
	Json::TryReadStringProperty(metadataText, "displayName", metadata.DisplayName);
	Json::TryReadStringProperty(metadataText, "source", metadata.SourcePath);
	return metadata;
}
};

std::optional<MeshDiagnosticMetadata> FindMeshDiagnosticMetadata(const MeshDiagnosticsRow& row)
{
	if (row.MeshAssetId == 0) return std::nullopt;
	static std::unordered_map<std::uint64_t, std::optional<MeshDiagnosticMetadata>> metadataCache;
	auto [metadataIt, inserted] = metadataCache.try_emplace(row.MeshAssetId);
	if (inserted) metadataIt->second = CookedMeshMetadataReader::LoadCookedMeshMetadata(row.MeshAssetId);
	return metadataIt->second;
}
