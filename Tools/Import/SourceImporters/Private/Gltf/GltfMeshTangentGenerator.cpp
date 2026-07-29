#include "PCH.h"

#include "Gltf/GltfMeshTangentGenerator.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cmath>
#include <cstdint>

void GltfMeshTangentGenerator::GenerateTangents(ImportedMeshGeometry& geometry)
{
	if (geometry.vertices.empty() || geometry.indices.empty() || geometry.indices.size() % 3u != 0u)
	{
		throw Diagnostics::Error("Cannot generate a tangent basis for incomplete triangle geometry.");
	}

	std::vector<DirectX::XMFLOAT3> tangentSums(geometry.vertices.size());
	std::vector<DirectX::XMFLOAT3> bitangentSums(geometry.vertices.size());
	for (std::size_t firstIndex = 0; firstIndex < geometry.indices.size(); firstIndex += 3u)
	{
		AccumulateTriangle(geometry, firstIndex, tangentSums, bitangentSums);
	}

	for (std::size_t vertexIndex = 0; vertexIndex < geometry.vertices.size(); ++vertexIndex)
	{
		BuildVertexTangent(geometry.vertices[vertexIndex], tangentSums[vertexIndex], bitangentSums[vertexIndex]);
	}
}

void GltfMeshTangentGenerator::AccumulateTriangle(
    const ImportedMeshGeometry& geometry,
    std::size_t firstIndex,
    std::vector<DirectX::XMFLOAT3>& tangentSums,
    std::vector<DirectX::XMFLOAT3>& bitangentSums)
{
	const std::uint32_t firstVertexIndex = geometry.indices[firstIndex];
	const std::uint32_t secondVertexIndex = geometry.indices[firstIndex + 1u];
	const std::uint32_t thirdVertexIndex = geometry.indices[firstIndex + 2u];
	if (firstVertexIndex >= geometry.vertices.size() || secondVertexIndex >= geometry.vertices.size() ||
	    thirdVertexIndex >= geometry.vertices.size())
	{
		throw Diagnostics::Error("Cannot generate tangents for a triangle with an out-of-range vertex index.");
	}

	const ImportedVertex& firstVertex = geometry.vertices[firstVertexIndex];
	const ImportedVertex& secondVertex = geometry.vertices[secondVertexIndex];
	const ImportedVertex& thirdVertex = geometry.vertices[thirdVertexIndex];
	const DirectX::XMFLOAT3 firstEdge{
	    secondVertex.position.x - firstVertex.position.x,
	    secondVertex.position.y - firstVertex.position.y,
	    secondVertex.position.z - firstVertex.position.z};
	const DirectX::XMFLOAT3 secondEdge{
	    thirdVertex.position.x - firstVertex.position.x,
	    thirdVertex.position.y - firstVertex.position.y,
	    thirdVertex.position.z - firstVertex.position.z};
	const DirectX::XMFLOAT2 firstUvEdge{secondVertex.uv.x - firstVertex.uv.x, secondVertex.uv.y - firstVertex.uv.y};
	const DirectX::XMFLOAT2 secondUvEdge{thirdVertex.uv.x - firstVertex.uv.x, thirdVertex.uv.y - firstVertex.uv.y};
	const float determinant = firstUvEdge.x * secondUvEdge.y - firstUvEdge.y * secondUvEdge.x;
	if (std::abs(determinant) <= 1.0e-8f)
	{
		return;
	}

	const float inverseDeterminant = 1.0f / determinant;
	const DirectX::XMFLOAT3 tangent{
	    (firstEdge.x * secondUvEdge.y - secondEdge.x * firstUvEdge.y) * inverseDeterminant,
	    (firstEdge.y * secondUvEdge.y - secondEdge.y * firstUvEdge.y) * inverseDeterminant,
	    (firstEdge.z * secondUvEdge.y - secondEdge.z * firstUvEdge.y) * inverseDeterminant};
	const DirectX::XMFLOAT3 bitangent{
	    (secondEdge.x * firstUvEdge.x - firstEdge.x * secondUvEdge.x) * inverseDeterminant,
	    (secondEdge.y * firstUvEdge.x - firstEdge.y * secondUvEdge.x) * inverseDeterminant,
	    (secondEdge.z * firstUvEdge.x - firstEdge.z * secondUvEdge.x) * inverseDeterminant};

	for (const std::uint32_t vertexIndex : {firstVertexIndex, secondVertexIndex, thirdVertexIndex})
	{
		tangentSums[vertexIndex].x += tangent.x;
		tangentSums[vertexIndex].y += tangent.y;
		tangentSums[vertexIndex].z += tangent.z;
		bitangentSums[vertexIndex].x += bitangent.x;
		bitangentSums[vertexIndex].y += bitangent.y;
		bitangentSums[vertexIndex].z += bitangent.z;
	}
}

void GltfMeshTangentGenerator::BuildVertexTangent(
    ImportedVertex& vertex,
    const DirectX::XMFLOAT3& tangentSum,
    const DirectX::XMFLOAT3& bitangentSum)
{
	const DirectX::XMVECTOR normal = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&vertex.normal));
	const DirectX::XMVECTOR accumulatedTangent = DirectX::XMLoadFloat3(&tangentSum);
	const DirectX::XMVECTOR projectedNormal =
	    DirectX::XMVectorScale(normal, DirectX::XMVectorGetX(DirectX::XMVector3Dot(normal, accumulatedTangent)));
	const DirectX::XMVECTOR orthogonalTangent = DirectX::XMVectorSubtract(accumulatedTangent, projectedNormal);
	const float tangentLengthSquared = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(orthogonalTangent));
	if (tangentLengthSquared <= 1.0e-8f)
	{
		throw Diagnostics::Error("Cannot generate a complete tangent basis from the source UVs.");
	}

	const DirectX::XMVECTOR tangent = DirectX::XMVector3Normalize(orthogonalTangent);
	const DirectX::XMVECTOR bitangent = DirectX::XMLoadFloat3(&bitangentSum);
	const float handedness =
	    DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) < 0.0f ? -1.0f : 1.0f;
	DirectX::XMFLOAT3 tangentDirection;
	DirectX::XMStoreFloat3(&tangentDirection, tangent);
	vertex.tangent = {tangentDirection.x, tangentDirection.y, tangentDirection.z, handedness};
}
