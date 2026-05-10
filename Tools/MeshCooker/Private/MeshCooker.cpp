#include "PCH.h"

#include "MeshCooker.h"

#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <filesystem>
#include <fstream>
#include <utility>

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
	}

	outErrorMessage.clear();
	return true;
}

Assets::CookedAssetId MeshCooker::BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept
{
	return Hash::Fnv1a64(std::string(sceneAssetId) + "#mesh#" + std::to_string(meshIndex));
}
