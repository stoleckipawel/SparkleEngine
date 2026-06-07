#include "PCH.h"

#include "Assets/Loaders/MeshAssetLoader.h"

#include "Assets/Cooked/CookedMeshAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Core/Public/Files/FileUtils.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace Assets
{
	bool MeshAssetLoader::Load(const std::filesystem::path& path, LoadedMeshAsset& outMeshAsset, std::string& outErrorMessage) const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
		{
			return false;
		}

		CookedAssetByteReader reader(fileBytes);
		CookedMeshAssetHeader header;
		if (!reader.Read(header, outErrorMessage))
		{
			return false;
		}

		if (!header.fileHeader.Matches(kCookedMeshAssetMagic, kCookedMeshAssetVersion) ||
		    !HasValidHeader(header.vertexStride, header.indexStride))
		{
			outErrorMessage = "Invalid cooked mesh asset header";
			return false;
		}

		const bool hasSkinInfluences = (header.flags & CookedMeshAssetFlag_HasSkinInfluences) != 0u;
		const bool isSkeletal = header.assetKind == CookedMeshAssetKind::Skeletal;
		if (header.assetKind != CookedMeshAssetKind::Static && header.assetKind != CookedMeshAssetKind::Skeletal)
		{
			outErrorMessage = "Invalid cooked mesh asset kind";
			return false;
		}

		if ((isSkeletal && (!hasSkinInfluences || header.skinInfluenceCount != header.vertexCount)) ||
		    (!isSkeletal && (hasSkinInfluences || header.skinInfluenceCount != 0u)) ||
		    header.skinInfluenceStride != sizeof(CookedMeshSkinInfluence))
		{
			outErrorMessage = "Invalid cooked mesh skin influence stream";
			return false;
		}

		std::vector<CookedMeshVertex> cookedVertices;
		std::vector<std::uint32_t> cookedIndices;
		std::vector<CookedMeshSkinInfluence> cookedSkinInfluences;
		if (!reader.ReadArray(header.vertexCount, cookedVertices, outErrorMessage) ||
		    !reader.ReadArray(header.indexCount, cookedIndices, outErrorMessage) ||
		    !reader.ReadArray(header.skinInfluenceCount, cookedSkinInfluences, outErrorMessage))
		{
			return false;
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			outErrorMessage = "Cooked mesh asset contains unexpected trailing bytes";
			return false;
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
		if (isSkeletal)
		{
			SkeletalMeshData loadedSkeletalMesh;
			loadedSkeletalMesh.geometry = std::move(geometry);
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
			outMeshAsset.payload = std::move(geometry);
		}

		outErrorMessage.clear();
		return true;
	}

	bool MeshAssetLoader::HasValidHeader(std::uint32_t vertexStride, std::uint32_t indexStride) noexcept
	{
		return vertexStride == sizeof(CookedMeshVertex) && indexStride == sizeof(std::uint32_t);
	}
}
