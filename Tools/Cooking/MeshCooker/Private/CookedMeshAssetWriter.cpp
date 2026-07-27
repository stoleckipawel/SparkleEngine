#include "PCH.h"

#include "CookedMeshAssetWriter.h"

#include "CookedMeshAssetBuild.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <cstdint>
#include <filesystem>
#include <fstream>

class CookedMeshAssetStager final
{
  public:
	static bool StageMeshAsset(
	    const CookedMeshAssetBuild& meshAsset,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static bool WriteMeshAsset(
	    const CookedMeshAssetBuild& meshAsset,
	    const std::filesystem::path& outputPath,
	    std::string& outErrorMessage);
	static bool WriteMeshMetadata(
	    const CookedMeshAssetBuild& meshAsset,
	    const std::filesystem::path& outputPath,
	    std::string& outErrorMessage);
	static Assets::CookedMeshAssetHeader BuildHeader(
	    const CookedMeshAssetBuild& meshAsset) noexcept;
	static std::filesystem::path BuildMetadataPath(
	    Assets::CookedAssetId assetId);
};

bool CookedMeshAssetStager::StageMeshAsset(
    const CookedMeshAssetBuild& meshAsset,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	const std::filesystem::path outputPath =
	    Paths::CookedMeshAsset(meshAsset.assetId);
	const std::filesystem::path stagedOutputPath =
	    Files::BuildTemporaryPath(outputPath, ".cook-generation");
	const std::filesystem::path metadataPath =
	    BuildMetadataPath(meshAsset.assetId);
	const std::filesystem::path stagedMetadataPath =
	    Files::BuildTemporaryPath(metadataPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedOutputPath);
	Files::CleanupTemporaryFile(stagedMetadataPath);

	outPublication.push_back({stagedOutputPath, outputPath});
	outPublication.push_back({stagedMetadataPath, metadataPath});

	return WriteMeshAsset(
	           meshAsset,
	           stagedOutputPath,
	           outErrorMessage) &&
	       WriteMeshMetadata(
	           meshAsset,
	           stagedMetadataPath,
	           outErrorMessage);
}

bool CookedMeshAssetStager::WriteMeshAsset(
    const CookedMeshAssetBuild& meshAsset,
    const std::filesystem::path& outputPath,
    std::string& outErrorMessage)
{
	const Assets::CookedMeshAssetHeader header = BuildHeader(meshAsset);

	std::ofstream output;
	if (!Files::TryOpenBinaryOutput(outputPath, output, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.vertices, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.indices, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.skinInfluences, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.morphTargets, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.morphTargetDeltas, outErrorMessage))
	{
		return false;
	}

	return Files::TryCloseOutput(
	    output,
	    outputPath,
	    outErrorMessage);
}

bool CookedMeshAssetStager::WriteMeshMetadata(
    const CookedMeshAssetBuild& meshAsset,
    const std::filesystem::path& outputPath,
    std::string& outErrorMessage)
{
	Json::ObjectWriter writer;
	writer.WriteString("schema", "cooked-mesh-metadata-v1");
	writer.WriteHexUInt64("assetId", meshAsset.assetId);
	writer.WriteString("displayName", meshAsset.displayName);
	writer.WriteString("source", meshAsset.sourcePath.generic_string());
	return Files::TryWriteAllText(
	    outputPath,
	    writer.Finish(),
	    outErrorMessage);
}

Assets::CookedMeshAssetHeader CookedMeshAssetStager::BuildHeader(
    const CookedMeshAssetBuild& meshAsset) noexcept
{
	return Assets::CookedMeshAssetHeader{
	    .fileHeader = {Assets::kCookedMeshAssetMagic, Assets::kCookedMeshAssetVersion},
	    .vertexCount = static_cast<std::uint32_t>(meshAsset.vertices.size()),
	    .indexCount = static_cast<std::uint32_t>(meshAsset.indices.size()),
	    .skinInfluenceCount = static_cast<std::uint32_t>(meshAsset.skinInfluences.size()),
	    .vertexStride = sizeof(Assets::CookedMeshVertex),
	    .indexStride = sizeof(std::uint32_t),
	    .skinInfluenceStride = sizeof(Assets::CookedMeshSkinInfluence),
	    .morphTargetCount = static_cast<std::uint32_t>(meshAsset.morphTargets.size()),
	    .morphTargetDeltaCount = static_cast<std::uint32_t>(meshAsset.morphTargetDeltas.size()),
	    .morphTargetRecordStride = sizeof(Assets::CookedMeshMorphTargetRecord),
	    .morphTargetDeltaStride = sizeof(Assets::CookedMeshMorphTargetDelta),
	    .flags = (meshAsset.HasSkinInfluences() ? Assets::CookedMeshAssetFlag_HasSkinInfluences : 0u) |
	             (meshAsset.HasMorphTargets() ? Assets::CookedMeshAssetFlag_HasMorphTargets : 0u),
	    .assetKind = meshAsset.assetKind};
}

std::filesystem::path CookedMeshAssetStager::BuildMetadataPath(
    Assets::CookedAssetId assetId)
{
	std::filesystem::path metadataPath = Paths::CookedMeshAsset(assetId);
	metadataPath += ".meta.json";
	return metadataPath;
}

bool CookedMeshAssetWriter::StageMeshAssets(
    const std::vector<CookedMeshAssetBuild>& meshAssets,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	for (const CookedMeshAssetBuild& meshAsset : meshAssets)
	{
		if (!CookedMeshAssetStager::StageMeshAsset(
		        meshAsset,
		        outPublication,
		        outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}
