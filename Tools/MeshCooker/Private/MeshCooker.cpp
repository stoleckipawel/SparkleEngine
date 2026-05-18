#include "PCH.h"

#include "MeshCooker.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

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
}

MeshCookOutput MeshCooker::BuildMeshAssets(const SourceImportResult& importResult, std::string_view sceneAssetId)
{
	MeshCookOutput output;
	output.assets.reserve(importResult.scene.meshPrimitives.size());
	output.assetReferences.reserve(importResult.scene.meshPrimitives.size());

	for (std::size_t primitiveIndex = 0; primitiveIndex < importResult.scene.meshPrimitives.size(); ++primitiveIndex)
	{
		const ImportedMeshPrimitive& importedPrimitive = importResult.scene.meshPrimitives[primitiveIndex];
		const ImportedMeshGeometry& meshGeometry = importedPrimitive.geometry;
		CookedMeshAssetBuild meshAsset;
		meshAsset.assetId = BuildMeshAssetId(sceneAssetId, primitiveIndex);
		meshAsset.displayName = importedPrimitive.displayName;
		meshAsset.sourcePath = importResult.scene.sourcePath;
		meshAsset.vertices.reserve(meshGeometry.vertices.size());
		for (const ImportedVertex& vertex : meshGeometry.vertices)
		{
			meshAsset.vertices.push_back(
			    Assets::CookedMeshVertex{
			        .position = vertex.position,
			        .uv = vertex.uv,
			        .color = vertex.color,
			        .normal = vertex.normal,
			        .tangent = vertex.tangent});
		}
		meshAsset.indices = meshGeometry.indices;

		output.assetReferences.push_back({meshAsset.assetId});
		output.assets.push_back(std::move(meshAsset));
	}

	return output;
}

bool MeshCooker::WriteMeshAssets(const std::vector<CookedMeshAssetBuild>& meshAssets, std::string& outErrorMessage)
{
	for (const CookedMeshAssetBuild& meshAsset : meshAssets)
	{
		const std::filesystem::path outputPath = Paths::CookedMeshAsset(meshAsset.assetId);
		const Assets::CookedMeshAssetHeader header{
		    .fileHeader = {Assets::kCookedMeshAssetMagic, Assets::kCookedMeshAssetVersion},
		    .vertexCount = static_cast<std::uint32_t>(meshAsset.vertices.size()),
		    .indexCount = static_cast<std::uint32_t>(meshAsset.indices.size()),
		    .vertexStride = sizeof(Assets::CookedMeshVertex),
		    .indexStride = sizeof(std::uint32_t)};
		std::ofstream output;
		if (!Files::TryOpenBinaryOutput(outputPath, output, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(output, header, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.vertices, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(output, meshAsset.indices, outErrorMessage))
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

Assets::CookedAssetId MeshCooker::BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept
{
	return Hash::Fnv1a64(std::string(sceneAssetId) + "#mesh#" + std::to_string(meshIndex));
}
