#include "PCH.h"

#include "Assets/Loaders/MeshAssetLoader.h"

#include "Assets/Cooked/CookedMeshAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include <algorithm>
#include <cstring>
#include <iterator>
#include <utility>

namespace Assets
{
	namespace MeshAssetLoaderDetail
	{
		template <typename T> T ReadRecord(std::span<const std::uint8_t> bytes, std::size_t index) noexcept
		{
			T value;
			std::memcpy(&value, bytes.data() + index * sizeof(T), sizeof(T));
			return value;
		}
	}

	LoadedMeshAsset MeshAssetLoader::Decode(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) const
	{
		const CookedAssetLoaderDiagnostics diagnostics(path, "CookedMeshAsset");

		CookedAssetByteReader reader(bytes);
		const CookedMeshAssetHeader header = reader.Read<CookedMeshAssetHeader>();

		if (!header.fileHeader.HasMagic(kCookedMeshAssetMagic) || header.vertexStride != sizeof(CookedMeshVertex)
		    || header.indexStride != sizeof(std::uint32_t))
		{
			throw diagnostics.MakeError(
			    "header",
			    "mesh magic and current vertex/index strides",
			    "Invalid cooked mesh asset header; recook the asset");
		}

		const bool hasSkinInfluences = (header.flags & static_cast<std::uint32_t>(CookedMeshAssetFlag::HasSkinInfluences)) != 0u;
		const bool hasMorphTargets = (header.flags & static_cast<std::uint32_t>(CookedMeshAssetFlag::HasMorphTargets)) != 0u;
		const bool isSkeletal = header.assetKind == CookedMeshAssetKind::Skeletal;
		if (header.assetKind != CookedMeshAssetKind::Static && header.assetKind != CookedMeshAssetKind::Skeletal)
		{
			throw diagnostics.MakeError("header.assetKind", "Static or Skeletal", "Invalid cooked mesh asset kind");
		}

		if ((isSkeletal && (!hasSkinInfluences || header.skinInfluenceCount != header.vertexCount))
		    || (!isSkeletal && (hasSkinInfluences || header.skinInfluenceCount != 0u))
		    || header.skinInfluenceStride != sizeof(CookedMeshSkinInfluence))
		{
			throw diagnostics.MakeError(
			    "skinInfluences",
			    "skeletal meshes have one influence record per vertex",
			    "Invalid cooked mesh skin influence stream");
		}
		if ((!isSkeletal && hasMorphTargets) || (hasMorphTargets && (header.morphTargetCount == 0u || header.morphTargetDeltaCount == 0u))
		    || (!hasMorphTargets && (header.morphTargetCount != 0u || header.morphTargetDeltaCount != 0u))
		    || header.morphTargetRecordStride != sizeof(CookedMeshMorphTargetRecord)
		    || header.morphTargetDeltaStride != sizeof(CookedMeshMorphTargetDelta))
		{
			throw diagnostics.MakeError(
			    "morphTargets",
			    "skeletal morph target flags, counts, and strides are consistent",
			    "Invalid cooked mesh morph target stream");
		}

		const std::span<const std::uint8_t> cookedVertices = reader.ReadArrayBytes<CookedMeshVertex>(header.vertexCount);
		MeshData geometry;
		geometry.vertices.resize(header.vertexCount);
		for (std::size_t vertexIndex = 0; vertexIndex < geometry.vertices.size(); ++vertexIndex)
		{
			const CookedMeshVertex cookedVertex = MeshAssetLoaderDetail::ReadRecord<CookedMeshVertex>(cookedVertices, vertexIndex);
			geometry.vertices[vertexIndex] =
			    VertexData(cookedVertex.position, cookedVertex.uv, cookedVertex.color, cookedVertex.normal, cookedVertex.tangent);
		}

		geometry.indices = reader.ReadArray<std::uint32_t>(header.indexCount);

		const std::span<const std::uint8_t> cookedSkinInfluences =
		    reader.ReadArrayBytes<CookedMeshSkinInfluence>(header.skinInfluenceCount);
		const std::vector<CookedMeshMorphTargetRecord> cookedMorphTargets =
		    reader.ReadArray<CookedMeshMorphTargetRecord>(header.morphTargetCount);
		const std::span<const std::uint8_t> cookedMorphTargetDeltas =
		    reader.ReadArrayBytes<CookedMeshMorphTargetDelta>(header.morphTargetDeltaCount);

		if (reader.GetRemainingByteCount() != 0)
		{
			throw diagnostics.MakeError(
			    "payload",
			    "no trailing bytes after declared mesh streams",
			    "Cooked mesh asset contains unexpected trailing bytes");
		}

		MeshMorphData morphTargets;
		morphTargets.targets.reserve(cookedMorphTargets.size());
		for (const CookedMeshMorphTargetRecord& cookedMorphTarget : cookedMorphTargets)
		{
			if (cookedMorphTarget.firstDelta > header.morphTargetDeltaCount
			    || cookedMorphTarget.deltaCount > header.morphTargetDeltaCount - cookedMorphTarget.firstDelta
			    || cookedMorphTarget.deltaCount != header.vertexCount)
			{
				throw diagnostics.MakeError(
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
				const CookedMeshMorphTargetDelta cookedDelta = MeshAssetLoaderDetail::ReadRecord<CookedMeshMorphTargetDelta>(
				    cookedMorphTargetDeltas,
				    cookedMorphTarget.firstDelta + deltaIndex);
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
			loadedSkeletalMesh.skinInfluences.resize(header.skinInfluenceCount);
			for (std::size_t influenceIndex = 0; influenceIndex < loadedSkeletalMesh.skinInfluences.size(); ++influenceIndex)
			{
				const CookedMeshSkinInfluence cookedInfluence =
				    MeshAssetLoaderDetail::ReadRecord<CookedMeshSkinInfluence>(cookedSkinInfluences, influenceIndex);
				VertexSkinInfluence& influence = loadedSkeletalMesh.skinInfluences[influenceIndex];
				std::copy(
				    std::begin(cookedInfluence.jointIndices),
				    std::end(cookedInfluence.jointIndices),
				    std::begin(influence.jointIndices));
				std::copy(
				    std::begin(cookedInfluence.jointWeights),
				    std::end(cookedInfluence.jointWeights),
				    std::begin(influence.jointWeights));
			}
			return LoadedMeshAsset{.payload = std::move(loadedSkeletalMesh)};
		}

		StaticMeshData loadedStaticMesh;
		loadedStaticMesh.geometry = std::move(geometry);
		return LoadedMeshAsset{.payload = std::move(loadedStaticMesh)};
	}
}
