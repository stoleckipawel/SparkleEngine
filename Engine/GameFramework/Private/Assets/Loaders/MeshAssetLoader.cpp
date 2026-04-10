#include "PCH.h"

#include "Assets/Loaders/MeshAssetLoader.h"

#include "Assets/Cooked/CookedMeshAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Core/Public/Files/FileUtils.h"

namespace Engine::Assets
{
	bool MeshAssetLoader::Load(const std::filesystem::path& path, MeshData& outMeshData, std::string& outErrorMessage) const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Engine::Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
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

		std::vector<CookedMeshVertex> cookedVertices;
		if (!reader.ReadArray(header.vertexCount, cookedVertices, outErrorMessage) ||
		    !reader.ReadArray(header.indexCount, outMeshData.indices, outErrorMessage))
		{
			return false;
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			outErrorMessage = "Cooked mesh asset contains unexpected trailing bytes";
			return false;
		}

		outMeshData.vertices.resize(cookedVertices.size());
		for (std::size_t vertexIndex = 0; vertexIndex < cookedVertices.size(); ++vertexIndex)
		{
			const CookedMeshVertex& cookedVertex = cookedVertices[vertexIndex];
			outMeshData.vertices[vertexIndex] = VertexData(
			    cookedVertex.position,
			    cookedVertex.uv,
			    cookedVertex.color,
			    cookedVertex.normal,
			    cookedVertex.tangent);
		}

		outErrorMessage.clear();
		return true;
	}

	bool MeshAssetLoader::HasValidHeader(std::uint32_t vertexStride, std::uint32_t indexStride) noexcept
	{
		return vertexStride == sizeof(CookedMeshVertex) && indexStride == sizeof(std::uint32_t);
	}
}