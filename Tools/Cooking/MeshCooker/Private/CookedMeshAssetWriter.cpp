#include "PCH.h"

#include "CookedMeshAssetWriter.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <cstdint>
#include <filesystem>
#include <fstream>

class CookedMeshAssetWriterOperations final
{
  public:
	static std::filesystem::path BuildCookedMeshMetadataPath(Assets::CookedAssetId assetId)
	{
		std::filesystem::path metadataPath = Paths::CookedMeshAsset(assetId);
		metadataPath += ".meta.json";
		return metadataPath;
	}

	static bool WriteMeshMetadata(
	    const CookedMeshAssetBuild& meshAsset,
	    const std::filesystem::path& outputPath,
	    std::string& outErrorMessage)
	{
		Json::ObjectWriter writer;
		writer.WriteString("schema", "cooked-mesh-metadata-v1");
		writer.WriteHexUInt64("assetId", meshAsset.assetId);
		writer.WriteString("displayName", meshAsset.displayName);
		writer.WriteString("source", meshAsset.sourcePath.generic_string());
		return Files::TryWriteAllText(outputPath, writer.Finish(), outErrorMessage);
	}
};

bool CookedMeshAssetWriter::StageMeshAssets(
    const std::vector<CookedMeshAssetBuild>& meshAssets,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	for (const CookedMeshAssetBuild& meshAsset : meshAssets)
	{
		const std::filesystem::path outputPath = Paths::CookedMeshAsset(meshAsset.assetId);
		const std::filesystem::path stagedOutputPath =
		    Files::BuildTemporaryPath(outputPath, ".cook-generation");
		const std::filesystem::path metadataPath =
		    CookedMeshAssetWriterOperations::BuildCookedMeshMetadataPath(meshAsset.assetId);
		const std::filesystem::path stagedMetadataPath =
		    Files::BuildTemporaryPath(metadataPath, ".cook-generation");

		Files::CleanupTemporaryFile(stagedOutputPath);
		Files::CleanupTemporaryFile(stagedMetadataPath);

		outPublication.push_back({stagedOutputPath, outputPath});
		outPublication.push_back({stagedMetadataPath, metadataPath});

		const Assets::CookedMeshAssetHeader header{
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

		std::ofstream output;
		if (!Files::TryOpenBinaryOutput(stagedOutputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.vertices, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.indices, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.skinInfluences, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.morphTargets, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.morphTargetDeltas, outErrorMessage))
		{
			return false;
		}

		if (!Files::TryCloseOutput(output, stagedOutputPath, outErrorMessage))
		{
			return false;
		}

		if (!CookedMeshAssetWriterOperations::WriteMeshMetadata(
		        meshAsset,
		        stagedMetadataPath,
		        outErrorMessage))
		{
			return false;
		}

	}

	outErrorMessage.clear();
	return true;
}
