#include "PCH.h"

#include "CookedMeshAssetBuilder.h"

#include "Core/Public/Hash/HashUtils.h"

#include <algorithm>
#include <string>
#include <cstring>
#include <utility>

class CookedMeshAssetBuilderOperations final
{
  public:
	static Assets::CookedAssetId BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#mesh#" + std::to_string(meshIndex));
	}

	static Assets::CookedMeshSkinInfluence BuildCookedSkinInfluence(const ImportedSkinInfluence& skinInfluence) noexcept
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

	static Assets::CookedMeshMorphTargetDelta BuildCookedMorphTargetDelta(const ImportedMorphTargetDelta& delta) noexcept
	{
		return Assets::CookedMeshMorphTargetDelta{
		    .position = delta.position,
		    .normal = delta.normal,
		    .tangent = delta.tangent};
	}

	static void CopyMorphTargetName(std::string_view sourceName, char (&outName)[Assets::kCookedMeshMorphTargetNameCapacity]) noexcept
	{
		const std::size_t copyLength = (std::min)(sourceName.size(), static_cast<std::size_t>(Assets::kCookedMeshMorphTargetNameCapacity - 1u));
		std::memcpy(outName, sourceName.data(), copyLength);
		outName[copyLength] = '\0';
	}
};

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
		meshAsset.assetId = CookedMeshAssetBuilderOperations::BuildMeshAssetId(sceneAssetId, primitiveIndex);
		meshAsset.displayName = importedPrimitive.displayName;
		meshAsset.sourcePath = importResult.scene.sourcePath;
		meshAsset.assetKind = meshGeometry.HasSkinInfluences() ? Assets::CookedMeshAssetKind::Skeletal : Assets::CookedMeshAssetKind::Static;
		meshAsset.vertices.reserve(meshGeometry.vertices.size());
		if (meshGeometry.HasSkinInfluences())
		{
			meshAsset.skinInfluences.reserve(meshGeometry.deformation.skinInfluences.size());
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
		}

		for (const ImportedSkinInfluence& skinInfluence : meshGeometry.deformation.skinInfluences)
		{
			meshAsset.skinInfluences.push_back(CookedMeshAssetBuilderOperations::BuildCookedSkinInfluence(skinInfluence));
		}
		meshAsset.indices = meshGeometry.indices;
		if (meshAsset.IsSkeletal())
		{
			meshAsset.morphTargets.reserve(meshGeometry.deformation.morphTargets.size());
			for (const ImportedMorphTarget& morphTarget : meshGeometry.deformation.morphTargets)
			{
				if (!morphTarget.IsValidForVertexCount(static_cast<std::uint32_t>(meshGeometry.vertices.size())))
				{
					continue;
				}

				Assets::CookedMeshMorphTargetRecord record;
				CookedMeshAssetBuilderOperations::CopyMorphTargetName(morphTarget.name, record.name);
				record.defaultWeight = morphTarget.defaultWeight;
				record.firstDelta = static_cast<std::uint32_t>(meshAsset.morphTargetDeltas.size());
				record.deltaCount = static_cast<std::uint32_t>(morphTarget.deltas.size());
				meshAsset.morphTargets.push_back(record);
				meshAsset.morphTargetDeltas.reserve(meshAsset.morphTargetDeltas.size() + morphTarget.deltas.size());
				for (const ImportedMorphTargetDelta& delta : morphTarget.deltas)
				{
					meshAsset.morphTargetDeltas.push_back(CookedMeshAssetBuilderOperations::BuildCookedMorphTargetDelta(delta));
				}
			}
		}

		output.assetReferences.push_back({meshAsset.assetId, meshAsset.assetKind});
		output.assets.push_back(std::move(meshAsset));
	}

	return output;
}
