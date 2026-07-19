#include "PCH.h"

#include "Assets/Loaders/MeshAssetLoader.h"

#include "Assets/Cooked/CookedMeshAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include <algorithm>
#include <iterator>
#include <utility>

namespace Assets
{
	bool MeshAssetLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes,
	    LoadedMeshAsset& outMeshAsset,
	    std::string& outErrorMessage) const
	{
		const CookedAssetLoaderContext diagnosticsContext =
		    CookedAssetLoaderDiagnostics::BuildContext(path, "CookedMeshAsset", kCookedMeshAssetVersion);
		auto fail = [&](std::string_view recordKind, std::string_view expectedFeature, std::string_view reason) -> bool
		{
			CookedAssetLoaderDiagnostics::SetFailure(diagnosticsContext, recordKind, expectedFeature, reason, outErrorMessage);
			return false;
		};

		CookedAssetByteReader reader(bytes);
		CookedMeshAssetHeader header;
		if (!reader.Read(header, outErrorMessage))
		{
			return fail("header", "CookedMeshAssetHeader", outErrorMessage);
		}

		if (!header.fileHeader.Matches(kCookedMeshAssetMagic, kCookedMeshAssetVersion) ||
		    !HasValidHeader(header.vertexStride, header.indexStride))
		{
			return fail("header", "mesh magic/version plus vertex and index strides", "Invalid cooked mesh asset header");
		}

		const bool hasSkinInfluences = (header.flags & CookedMeshAssetFlag_HasSkinInfluences) != 0u;
		const bool hasMorphTargets = (header.flags & CookedMeshAssetFlag_HasMorphTargets) != 0u;
		const bool isSkeletal = header.assetKind == CookedMeshAssetKind::Skeletal;
		if (header.assetKind != CookedMeshAssetKind::Static && header.assetKind != CookedMeshAssetKind::Skeletal)
		{
			return fail("header.assetKind", "Static or Skeletal", "Invalid cooked mesh asset kind");
		}

		if ((isSkeletal && (!hasSkinInfluences || header.skinInfluenceCount != header.vertexCount)) ||
		    (!isSkeletal && (hasSkinInfluences || header.skinInfluenceCount != 0u)) ||
		    header.skinInfluenceStride != sizeof(CookedMeshSkinInfluence))
		{
			return fail("skinInfluences", "skeletal meshes have one influence record per vertex", "Invalid cooked mesh skin influence stream");
		}
		if ((!isSkeletal && hasMorphTargets) ||
		    (hasMorphTargets && (header.morphTargetCount == 0u || header.morphTargetDeltaCount == 0u)) ||
		    (!hasMorphTargets && (header.morphTargetCount != 0u || header.morphTargetDeltaCount != 0u)) ||
		    header.morphTargetRecordStride != sizeof(CookedMeshMorphTargetRecord) ||
		    header.morphTargetDeltaStride != sizeof(CookedMeshMorphTargetDelta))
		{
			return fail("morphTargets", "skeletal morph target flags, counts, and strides are consistent", "Invalid cooked mesh morph target stream");
		}

		std::vector<CookedMeshVertex> cookedVertices;
		std::vector<std::uint32_t> cookedIndices;
		std::vector<CookedMeshSkinInfluence> cookedSkinInfluences;
		std::vector<CookedMeshMorphTargetRecord> cookedMorphTargets;
		std::vector<CookedMeshMorphTargetDelta> cookedMorphTargetDeltas;
		if (!reader.ReadArray(header.vertexCount, cookedVertices, outErrorMessage) ||
		    !reader.ReadArray(header.indexCount, cookedIndices, outErrorMessage) ||
		    !reader.ReadArray(header.skinInfluenceCount, cookedSkinInfluences, outErrorMessage) ||
		    !reader.ReadArray(header.morphTargetCount, cookedMorphTargets, outErrorMessage) ||
		    !reader.ReadArray(header.morphTargetDeltaCount, cookedMorphTargetDeltas, outErrorMessage))
		{
			return fail("streams", "vertex/index/skinning/morph arrays matching header counts", outErrorMessage);
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			return fail("payload", "no trailing bytes after declared mesh streams", "Cooked mesh asset contains unexpected trailing bytes");
		}

		MeshData geometry;
		geometry.indices = std::move(cookedIndices);
		geometry.vertices.resize(cookedVertices.size());
		for (std::size_t vertexIndex = 0; vertexIndex < cookedVertices.size(); ++vertexIndex)
		{
			const CookedMeshVertex& cookedVertex = cookedVertices[vertexIndex];
			geometry.vertices[vertexIndex] =
			    VertexData(cookedVertex.position, cookedVertex.uv, cookedVertex.color, cookedVertex.normal, cookedVertex.tangent);
		}
		MeshMorphData morphTargets;
		morphTargets.targets.reserve(cookedMorphTargets.size());
		for (const CookedMeshMorphTargetRecord& cookedMorphTarget : cookedMorphTargets)
		{
			if (cookedMorphTarget.firstDelta > cookedMorphTargetDeltas.size() ||
			    cookedMorphTarget.deltaCount > cookedMorphTargetDeltas.size() - cookedMorphTarget.firstDelta ||
			    cookedMorphTarget.deltaCount != header.vertexCount)
			{
				return fail(
				    "morphTargets.deltaRange",
				    "each morph target references one delta range matching vertex count",
				    "Cooked mesh morph target references an invalid delta range");
			}

			MeshMorphTarget morphTarget;
			morphTarget.name = cookedMorphTarget.name;
			morphTarget.defaultWeight = cookedMorphTarget.defaultWeight;
			morphTarget.deltas.resize(cookedMorphTarget.deltaCount);
			for (std::uint32_t deltaIndex = 0; deltaIndex < cookedMorphTarget.deltaCount; ++deltaIndex)
			{
				const CookedMeshMorphTargetDelta& cookedDelta = cookedMorphTargetDeltas[cookedMorphTarget.firstDelta + deltaIndex];
				morphTarget.deltas[deltaIndex] =
				    MeshMorphTargetDelta{.position = cookedDelta.position, .normal = cookedDelta.normal, .tangent = cookedDelta.tangent};
			}
			morphTargets.targets.push_back(std::move(morphTarget));
		}
		if (isSkeletal)
		{
			SkeletalMeshData loadedSkeletalMesh;
			loadedSkeletalMesh.geometry = std::move(geometry);
			loadedSkeletalMesh.morphTargets = std::move(morphTargets);
			loadedSkeletalMesh.skinInfluences.resize(cookedSkinInfluences.size());
			for (std::size_t influenceIndex = 0; influenceIndex < cookedSkinInfluences.size(); ++influenceIndex)
			{
				const CookedMeshSkinInfluence& cookedInfluence = cookedSkinInfluences[influenceIndex];
				VertexSkinInfluence& influence = loadedSkeletalMesh.skinInfluences[influenceIndex];
				std::copy(std::begin(cookedInfluence.jointIndices), std::end(cookedInfluence.jointIndices), std::begin(influence.jointIndices));
				std::copy(std::begin(cookedInfluence.jointWeights), std::end(cookedInfluence.jointWeights), std::begin(influence.jointWeights));
			}
			outMeshAsset.payload = std::move(loadedSkeletalMesh);
		}
		else
		{
			StaticMeshData loadedStaticMesh;
			loadedStaticMesh.geometry = std::move(geometry);
			outMeshAsset.payload = std::move(loadedStaticMesh);
		}

		outErrorMessage.clear();
		return true;
	}

	bool MeshAssetLoader::HasValidHeader(std::uint32_t vertexStride, std::uint32_t indexStride) noexcept
	{
		return vertexStride == sizeof(CookedMeshVertex) && indexStride == sizeof(std::uint32_t);
	}
}
