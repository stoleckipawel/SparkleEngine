#include "PCH.h"

#include "CookedMeshAssetWriter.h"

#include "CookedMeshAssetBuild.h"
#include "Core/Public/Diagnostics/Error.h"
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
	static void StageMeshAsset(const CookedMeshAssetBuild& meshAsset, std::vector<Files::FilePublication>& outPublication);
	static void WriteMeshAsset(const CookedMeshAssetBuild& meshAsset, const std::filesystem::path& outputPath);
	static void WriteMeshMetadata(const CookedMeshAssetBuild& meshAsset, const std::filesystem::path& outputPath);
	static Assets::CookedMeshAssetHeader BuildHeader(const CookedMeshAssetBuild& meshAsset) noexcept;
	static std::filesystem::path BuildMetadataPath(Assets::CookedAssetId assetId);
};

void CookedMeshAssetStager::StageMeshAsset(const CookedMeshAssetBuild& meshAsset, std::vector<Files::FilePublication>& outPublication)
{
	const std::filesystem::path outputPath = Paths::CookedMeshAsset(meshAsset.assetId);
	const std::filesystem::path stagedOutputPath = Files::BuildTemporaryPath(outputPath, ".cook-generation");
	const std::filesystem::path metadataPath = BuildMetadataPath(meshAsset.assetId);
	const std::filesystem::path stagedMetadataPath = Files::BuildTemporaryPath(metadataPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedOutputPath);
	Files::CleanupTemporaryFile(stagedMetadataPath);

	outPublication.push_back({stagedOutputPath, outputPath});
	outPublication.push_back({stagedMetadataPath, metadataPath});

	WriteMeshAsset(meshAsset, stagedOutputPath);
	WriteMeshMetadata(meshAsset, stagedMetadataPath);
}

void CookedMeshAssetStager::WriteMeshAsset(const CookedMeshAssetBuild& meshAsset, const std::filesystem::path& outputPath)
{
	const Assets::CookedMeshAssetHeader header = BuildHeader(meshAsset);

	std::string errorMessage;
	std::ofstream output;
	if (!Files::TryOpenBinaryOutput(outputPath, output, errorMessage)
	    || !Files::BinaryStreamWriter::WriteValue(output, header, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(output, meshAsset.vertices, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(output, meshAsset.indices, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(output, meshAsset.skinInfluences, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(output, meshAsset.morphTargets, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(output, meshAsset.morphTargetDeltas, errorMessage))
	{
		throw Diagnostics::Error(std::move(errorMessage));
	}

	if (!Files::TryCloseOutput(output, outputPath, errorMessage))
	{
		throw Diagnostics::Error(std::move(errorMessage));
	}
}

void CookedMeshAssetStager::WriteMeshMetadata(const CookedMeshAssetBuild& meshAsset, const std::filesystem::path& outputPath)
{
	Json::ObjectWriter writer;
	writer.WriteString("schema", "cooked-mesh-metadata");
	writer.WriteHexUInt64("assetId", meshAsset.assetId);
	writer.WriteString("displayName", meshAsset.displayName);
	writer.WriteString("source", meshAsset.sourcePath.generic_string());
	std::string errorMessage;
	if (!Files::TryWriteAllText(outputPath, writer.Finish(), errorMessage))
	{
		throw Diagnostics::Error(std::move(errorMessage));
	}
}

Assets::CookedMeshAssetHeader CookedMeshAssetStager::BuildHeader(const CookedMeshAssetBuild& meshAsset) noexcept
{
	return Assets::CookedMeshAssetHeader{
	    .fileHeader = {Assets::kCookedMeshAssetMagic},
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
	    .flags = (meshAsset.HasSkinInfluences() ? Assets::CookedMeshAssetFlag_HasSkinInfluences : 0u)
	        | (meshAsset.HasMorphTargets() ? Assets::CookedMeshAssetFlag_HasMorphTargets : 0u),
	    .assetKind = meshAsset.assetKind};
}

std::filesystem::path CookedMeshAssetStager::BuildMetadataPath(Assets::CookedAssetId assetId)
{
	std::filesystem::path metadataPath = Paths::CookedMeshAsset(assetId);
	metadataPath += ".meta.json";
	return metadataPath;
}

void CookedMeshAssetWriter::StageMeshAssets(
    const std::vector<CookedMeshAssetBuild>& meshAssets,
    std::vector<Files::FilePublication>& outPublication)
{
	for (const CookedMeshAssetBuild& meshAsset : meshAssets)
	{
		CookedMeshAssetStager::StageMeshAsset(meshAsset, outPublication);
	}
}
