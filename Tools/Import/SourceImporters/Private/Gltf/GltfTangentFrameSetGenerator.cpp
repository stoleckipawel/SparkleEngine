#include "PCH.h"

#include "Gltf/GltfTangentFrameSetGenerator.h"

#include "Gltf/GltfMikkTangentContext.h"
#include "Gltf/GltfVertexFrameBuilder.h"
#include "Core/Public/Diagnostics/Error.h"

#include <format>
#include <utility>

GltfGeneratedTangentFrameSet GltfTangentFrameSetGenerator::Generate(const ImportedMeshGeometry& geometry)
{
	GltfGeneratedTangentFrameSet frames;
	frames.baseCornerTangents = GltfMikkTangentContext(geometry.vertices, geometry.indices).Generate();
	frames.morphCornerTangentDeltas.reserve(geometry.deformation.morphTargets.size());
	for (std::size_t targetIndex = 0; targetIndex < geometry.deformation.morphTargets.size(); ++targetIndex)
	{
		try
		{
			const std::vector<ImportedVertex> targetVertices = BuildMorphVertices(geometry, geometry.deformation.morphTargets[targetIndex]);
			const std::vector<DirectX::XMFLOAT4> targetTangents = GltfMikkTangentContext(targetVertices, geometry.indices).Generate();
			frames.morphCornerTangentDeltas.push_back(BuildMorphTangentDeltas(frames.baseCornerTangents, targetTangents, targetIndex));
		}
		catch (const Diagnostics::Error& error)
		{
			throw Diagnostics::Error(std::format("glTF morph target {} tangent generation failed: {}", targetIndex, error.what()));
		}
	}
	return frames;
}

std::vector<ImportedVertex> GltfTangentFrameSetGenerator::BuildMorphVertices(
    const ImportedMeshGeometry& geometry,
    const ImportedMorphTarget& morphTarget)
{
	if (morphTarget.deltas.size() != geometry.vertices.size())
	{
		throw Diagnostics::Error("Cannot generate tangents for incomplete glTF morph-target data.");
	}

	std::vector<ImportedVertex> vertices = geometry.vertices;
	for (std::size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
	{
		ImportedVertex& vertex = vertices[vertexIndex];
		const ImportedMorphTargetDelta& delta = morphTarget.deltas[vertexIndex];
		vertex.position.x += delta.position.x;
		vertex.position.y += delta.position.y;
		vertex.position.z += delta.position.z;
		vertex.normal.x += delta.normal.x;
		vertex.normal.y += delta.normal.y;
		vertex.normal.z += delta.normal.z;
		try
		{
			vertex.normal = GltfVertexFrameBuilder::BuildNormal(vertex.normal);
		}
		catch (const Diagnostics::Error& error)
		{
			throw Diagnostics::Error(std::format("vertex {} frame validation failed: {}", vertexIndex, error.what()));
		}
	}
	return vertices;
}

std::vector<DirectX::XMFLOAT3> GltfTangentFrameSetGenerator::BuildMorphTangentDeltas(
    const std::vector<DirectX::XMFLOAT4>& baseTangents,
    const std::vector<DirectX::XMFLOAT4>& targetTangents,
    std::size_t targetIndex)
{
	if (targetTangents.size() != baseTangents.size())
	{
		throw Diagnostics::Error("MikkTSpace produced inconsistent base and morph-target corner streams.");
	}

	std::vector<DirectX::XMFLOAT3> deltas(baseTangents.size());
	for (std::size_t cornerIndex = 0; cornerIndex < baseTangents.size(); ++cornerIndex)
	{
		const DirectX::XMFLOAT4& base = baseTangents[cornerIndex];
		const DirectX::XMFLOAT4& target = targetTangents[cornerIndex];
		if (target.w != base.w)
		{
			throw Diagnostics::Error(
			    std::format(
			        "glTF morph target {} changes tangent handedness at triangle {}, corner {}; split or repair the source mesh.",
			        targetIndex,
			        cornerIndex / 3u,
			        cornerIndex % 3u));
		}
		deltas[cornerIndex] = {target.x - base.x, target.y - base.y, target.z - base.z};
	}
	return deltas;
}
