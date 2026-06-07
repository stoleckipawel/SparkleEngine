#include "PCH.h"

#include "CookedMeshAssetWriter.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace
{
	std::filesystem::path BuildCookedMeshMetadataPath(Assets::CookedAssetId assetId)
	{
		std::filesystem::path metadataPath = Paths::CookedMeshAsset(assetId);
		metadataPath += ".meta.json";
		return metadataPath;
	}

	bool WriteMeshMetadata(const CookedMeshAssetBuild& meshAsset, std::string& outErrorMessage)
	{
		Json::ObjectWriter writer;
		writer.WriteString("schema", "cooked-mesh-metadata-v1");
		writer.WriteHexUInt64("assetId", meshAsset.assetId);
		writer.WriteString("displayName", meshAsset.displayName);
		writer.WriteString("source", meshAsset.sourcePath.generic_string());
		return Files::TryWriteAllTextAtomic(BuildCookedMeshMetadataPath(meshAsset.assetId), writer.Finish(), outErrorMessage);
	}
}  // namespace

bool CookedMeshAssetWriter::WriteMeshAssets(const std::vector<CookedMeshAssetBuild>& meshAssets, std::string& outErrorMessage)
{
	for (const CookedMeshAssetBuild& meshAsset : meshAssets)
	{
		const std::filesystem::path outputPath = Paths::CookedMeshAsset(meshAsset.assetId);
		const Assets::CookedMeshAssetHeader header{
		    .fileHeader = {Assets::kCookedMeshAssetMagic, Assets::kCookedMeshAssetVersion},
		    .vertexCount = static_cast<std::uint32_t>(meshAsset.vertices.size()),
		    .indexCount = static_cast<std::uint32_t>(meshAsset.indices.size()),
		    .skinInfluenceCount = static_cast<std::uint32_t>(meshAsset.skinInfluences.size()),
		    .vertexStride = sizeof(Assets::CookedMeshVertex),
		    .indexStride = sizeof(std::uint32_t),
		    .skinInfluenceStride = sizeof(Assets::CookedMeshSkinInfluence),
		    .flags = meshAsset.HasSkinInfluences() ? Assets::CookedMeshAssetFlag_HasSkinInfluences : 0u,
		    .assetKind = meshAsset.assetKind};
		std::ofstream output;
		if (!Files::TryOpenBinaryOutput(outputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.vertices, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.indices, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.skinInfluences, outErrorMessage))
		{
			return false;
		}

		if (!Files::TryCloseOutput(output, outputPath, outErrorMessage))
		{
			return false;
		}

		if (!WriteMeshMetadata(meshAsset, outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}
