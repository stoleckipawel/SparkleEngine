#include "PCH.h"

#include "CookedMeshAssetBuilder.h"

#include "Core/Public/Hash/HashUtils.h"

#include <string>
#include <utility>

namespace
{
	Assets::CookedAssetId BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#mesh#" + std::to_string(meshIndex));
	}

	Assets::CookedMeshSkinInfluence BuildCookedSkinInfluence(const ImportedSkinInfluence& skinInfluence) noexcept
	{
		return Assets::CookedMeshSkinInfluence{
		    .jointIndices =
		        {skinInfluence.jointIndices[0],
		         skinInfluence.jointIndices[1],
		         skinInfluence.jointIndices[2],
		         skinInfluence.jointIndices[3]},
		    .jointWeights =
		        {skinInfluence.jointWeights[0],
		         skinInfluence.jointWeights[1],
		         skinInfluence.jointWeights[2],
		         skinInfluence.jointWeights[3]}};
	}
}  // namespace

MeshCookOutput CookedMeshAssetBuilder::BuildMeshAssets(const SourceImportResult& importResult, std::string_view sceneAssetId)
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
		meshAsset.assetKind = meshGeometry.hasSkinInfluences ? Assets::CookedMeshAssetKind::Skeletal : Assets::CookedMeshAssetKind::Static;
		meshAsset.vertices.reserve(meshGeometry.vertices.size());
		if (meshGeometry.hasSkinInfluences)
		{
			meshAsset.skinInfluences.reserve(meshGeometry.vertices.size());
		}

		for (const ImportedVertex& vertex : meshGeometry.vertices)
		{
			meshAsset.vertices.push_back(
			    Assets::CookedMeshVertex{
			        .position = vertex.position,
			        .uv = vertex.uv,
			        .color = vertex.color,
			        .normal = vertex.normal,
			        .tangent = vertex.tangent});
			if (meshGeometry.hasSkinInfluences)
			{
				meshAsset.skinInfluences.push_back(BuildCookedSkinInfluence(vertex.skinInfluence));
			}
		}
		meshAsset.indices = meshGeometry.indices;

		output.assetReferences.push_back({meshAsset.assetId, meshAsset.assetKind});
		output.assets.push_back(std::move(meshAsset));
	}

	return output;
}
