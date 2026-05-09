#include "PCH.h"

#include "MeshCooker.h"

#include "CookArtifactCache.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <filesystem>
#include <fstream>
#include <utility>

static constexpr std::uint32_t kMeshCookerVersion = 1;

static Cook::CookArtifactKey BuildMeshCookArtifactKey(
    const CookedMeshAssetBuild& meshAsset,
    const Assets::CookedMeshAssetHeader& header,
    const std::filesystem::path& outputPath)
{
	std::uint64_t contentHash = Hash::ContinueFnv1a64Value(Hash::kFnv64OffsetBasis, header);
	contentHash = Hash::ContinueFnv1a64Vector(contentHash, meshAsset.vertices);
	contentHash = Hash::ContinueFnv1a64Vector(contentHash, meshAsset.indices);

	return Cook::CookArtifactKey{
	    .assetType = "Mesh",
	    .assetId = Formatting::FormatHexUInt64(meshAsset.assetId),
	    .cookerName = "MeshCooker",
	    .outputPath = outputPath,
	    .cookedFormatVersion = Assets::kCookedMeshAssetVersion,
	    .cookerVersion = kMeshCookerVersion,
	    .sourceHash = Hash::FinalizeFnv1a64(contentHash),
	    .dependencyHash = 0,
	    .settingsHash = Cook::CookArtifactCache::ComputeSettingsHash("CookedMeshAsset")};
}

void MeshCooker::BuildMeshAssets(
    const SourceImportResult& importResult,
    std::string_view sceneAssetId,
    std::vector<CookedMeshAssetBuild>& outMeshAssets,
    std::vector<Assets::CookedSceneMeshAssetRef>& outMeshAssetReferences)
{
	outMeshAssets.clear();
	outMeshAssetReferences.clear();
	outMeshAssets.reserve(importResult.meshes.size());
	outMeshAssetReferences.reserve(importResult.meshes.size());

	for (std::size_t meshIndex = 0; meshIndex < importResult.meshes.size(); ++meshIndex)
	{
		const MeshData& meshData = importResult.meshes[meshIndex];
		CookedMeshAssetBuild meshAsset;
		meshAsset.assetId = BuildMeshAssetId(sceneAssetId, meshIndex);
		meshAsset.vertices.reserve(meshData.vertices.size());
		for (const VertexData& vertex : meshData.vertices)
		{
			meshAsset.vertices.push_back(
			    Assets::CookedMeshVertex{
			        .position = vertex.position,
			        .uv = vertex.uv,
			        .color = vertex.color,
			        .normal = vertex.normal,
			        .tangent = vertex.tangent});
		}
		meshAsset.indices = meshData.indices;

		outMeshAssetReferences.push_back({meshAsset.assetId});
		outMeshAssets.push_back(std::move(meshAsset));
	}
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
		const Cook::CookArtifactKey artifactKey = BuildMeshCookArtifactKey(meshAsset, header, outputPath);
		bool isCurrent = false;
		isCurrent = Cook::CookArtifactCache::IsCurrent(artifactKey, outErrorMessage);
		if (!isCurrent && !outErrorMessage.empty())
		{
			return false;
		}
		if (isCurrent)
		{
			continue;
		}

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

		if (!Cook::CookArtifactCache::Publish(artifactKey, outErrorMessage))
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
