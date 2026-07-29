#include "PCH.h"

#include "CookedMeshAssetBuilder.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Hash/HashUtils.h"

#include <cstring>
#include <format>
#include <limits>
#include <string>
#include <utility>

class CookedMeshAssetTranslation final
{
  public:
	static CookedMeshAssetBuild BuildAsset(
	    const ImportedMeshPrimitive& importedPrimitive,
	    std::size_t primitiveIndex,
	    std::string_view sceneAssetId,
	    const std::filesystem::path& sourcePath)
	{
		const ImportedMeshGeometry& geometry = importedPrimitive.geometry;
		ValidateGeometry(geometry, primitiveIndex);

		CookedMeshAssetBuild asset;
		asset.assetId = BuildMeshAssetId(sceneAssetId, primitiveIndex);
		asset.displayName = importedPrimitive.displayName;
		asset.sourcePath = sourcePath;
		asset.assetKind = geometry.HasSkinInfluences() ? Assets::CookedMeshAssetKind::Skeletal : Assets::CookedMeshAssetKind::Static;
		asset.vertices.reserve(geometry.vertices.size());
		asset.skinInfluences.reserve(geometry.deformation.skinInfluences.size());

		for (const ImportedVertex& vertex : geometry.vertices)
		{
			asset.vertices.push_back(
			    Assets::CookedMeshVertex{
			        .position = vertex.position,
			        .uv = vertex.uv,
			        .color = vertex.color,
			        .normal = vertex.normal,
			        .tangent = vertex.tangent});
		}
		for (const ImportedSkinInfluence& skinInfluence : geometry.deformation.skinInfluences)
		{
			asset.skinInfluences.push_back(BuildCookedSkinInfluence(skinInfluence));
		}
		asset.indices = geometry.indices;
		AppendMorphTargets(geometry, primitiveIndex, asset);

		return asset;
	}

  private:
	static Assets::CookedAssetId BuildMeshAssetId(std::string_view sceneAssetId, std::size_t meshIndex) noexcept
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#mesh#" + std::to_string(meshIndex));
	}

	static void ValidateGeometry(const ImportedMeshGeometry& geometry, std::size_t primitiveIndex)
	{
		if (!geometry.IsValid() || geometry.vertices.size() > (std::numeric_limits<std::uint32_t>::max)() ||
		    geometry.indices.size() % 3u != 0u)
		{
			throw Diagnostics::Error(std::format("Imported mesh primitive {} has invalid triangle geometry.", primitiveIndex));
		}
		for (const std::uint32_t index : geometry.indices)
		{
			if (index >= geometry.vertices.size())
			{
				throw Diagnostics::Error(std::format(
				    "Imported mesh primitive {} references vertex {} but only {} vertices exist.",
				    primitiveIndex,
				    index,
				    geometry.vertices.size()));
			}
		}
		if (geometry.HasSkinInfluences() && geometry.deformation.skinInfluences.size() != geometry.vertices.size())
		{
			throw Diagnostics::Error(std::format(
			    "Imported mesh primitive {} has {} skin influences for {} vertices.",
			    primitiveIndex,
			    geometry.deformation.skinInfluences.size(),
			    geometry.vertices.size()));
		}
		if (geometry.HasMorphTargets() && !geometry.HasSkinInfluences())
		{
			throw Diagnostics::Error(
			    std::format("Imported mesh primitive {} has morph targets but no skeletal binding.", primitiveIndex));
		}
	}

	static Assets::CookedMeshSkinInfluence BuildCookedSkinInfluence(const ImportedSkinInfluence& skinInfluence) noexcept
	{
		return Assets::CookedMeshSkinInfluence{
		    .jointIndices =
		        {skinInfluence.jointIndices[0],
		         skinInfluence.jointIndices[1],
		         skinInfluence.jointIndices[2],
		         skinInfluence.jointIndices[3],
		         skinInfluence.jointIndices[4],
		         skinInfluence.jointIndices[5],
		         skinInfluence.jointIndices[6],
		         skinInfluence.jointIndices[7]},
		    .jointWeights = {
		        skinInfluence.jointWeights[0],
		        skinInfluence.jointWeights[1],
		        skinInfluence.jointWeights[2],
		        skinInfluence.jointWeights[3],
		        skinInfluence.jointWeights[4],
		        skinInfluence.jointWeights[5],
		        skinInfluence.jointWeights[6],
		        skinInfluence.jointWeights[7]}};
	}

	static Assets::CookedMeshMorphTargetDelta BuildCookedMorphTargetDelta(const ImportedMorphTargetDelta& delta) noexcept
	{
		return Assets::CookedMeshMorphTargetDelta{.position = delta.position, .normal = delta.normal, .tangent = delta.tangent};
	}

	static void CopyMorphTargetName(std::string_view sourceName, char (&outName)[Assets::kCookedMeshMorphTargetNameCapacity]) noexcept
	{
		std::memcpy(outName, sourceName.data(), sourceName.size());
		outName[sourceName.size()] = '\0';
	}

	static void AppendMorphTargets(
	    const ImportedMeshGeometry& geometry,
	    std::size_t primitiveIndex,
	    CookedMeshAssetBuild& asset)
	{
		asset.morphTargets.reserve(geometry.deformation.morphTargets.size());
		for (const ImportedMorphTarget& morphTarget : geometry.deformation.morphTargets)
		{
			if (!morphTarget.IsValidForVertexCount(static_cast<std::uint32_t>(geometry.vertices.size())))
			{
				throw Diagnostics::Error(std::format(
				    "Imported mesh primitive {} has a morph target with {} deltas for {} vertices.",
				    primitiveIndex,
				    morphTarget.deltas.size(),
				    geometry.vertices.size()));
			}
			if (morphTarget.name.size() >= Assets::kCookedMeshMorphTargetNameCapacity)
			{
				throw Diagnostics::Error(
				    std::format("Imported mesh primitive {} has a morph-target name that exceeds the cooked format.", primitiveIndex));
			}

			Assets::CookedMeshMorphTargetRecord record;
			CopyMorphTargetName(morphTarget.name, record.name);
			record.defaultWeight = morphTarget.defaultWeight;
			record.firstDelta = static_cast<std::uint32_t>(asset.morphTargetDeltas.size());
			record.deltaCount = static_cast<std::uint32_t>(morphTarget.deltas.size());
			asset.morphTargets.push_back(record);
			asset.morphTargetDeltas.reserve(asset.morphTargetDeltas.size() + morphTarget.deltas.size());
			for (const ImportedMorphTargetDelta& delta : morphTarget.deltas)
			{
				asset.morphTargetDeltas.push_back(BuildCookedMorphTargetDelta(delta));
			}
		}
	}
};

MeshCookOutput CookedMeshAssetBuilder::BuildMeshAssets(const SourceImportOutput& importOutput, std::string_view sceneAssetId)
{
	MeshCookOutput output;
	output.assets.reserve(importOutput.scene.meshPrimitives.size());
	output.assetReferences.reserve(importOutput.scene.meshPrimitives.size());

	for (std::size_t primitiveIndex = 0; primitiveIndex < importOutput.scene.meshPrimitives.size(); ++primitiveIndex)
	{
		const ImportedMeshPrimitive& importedPrimitive = importOutput.scene.meshPrimitives[primitiveIndex];
		CookedMeshAssetBuild meshAsset = CookedMeshAssetTranslation::BuildAsset(
		    importedPrimitive,
		    primitiveIndex,
		    sceneAssetId,
		    importOutput.GetSourcePath());
		output.assetReferences.push_back({meshAsset.assetId, meshAsset.assetKind});
		output.assets.push_back(std::move(meshAsset));
	}

	return output;
}
