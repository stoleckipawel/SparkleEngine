#include "PCH.h"
#include "Meshes/GpuMeshPreparation.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshData.h"
#include "Scene/Meshes/MeshMorphData.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"

#include <algorithm>

std::uint64_t GpuMeshPreparedData::GetDecodedByteSize() const noexcept
{
	return RayTracingVertices.size() * sizeof(RayTracingHitVertex) + RayTracingIndices.size() * sizeof(std::uint32_t)
	    + SkinInfluences.size() * sizeof(VertexSkinInfluence) + GpuSkinInfluences.size() * sizeof(VertexSkinInfluenceData)
	    + MorphTargetDeltas.size() * sizeof(MorphTargetDeltaData);
}

std::uint64_t GpuMeshPreparedData::GetResidentByteSize() const noexcept
{
	const MeshData& meshData = Source.GetResource()->GetMeshData();
	return GetDecodedByteSize() + meshData.GetVertexBufferSize() + meshData.GetIndexBufferSize()
	    + (std::max<std::size_t>(MorphTargetDeltas.size(), 1u) * sizeof(MorphTargetDeltaData));
}

GpuMeshPreparedData GpuMeshPreparation::Build(const ImmutableRenderMeshHandle& source)
{
	if (!source.IsValid())
		throw Diagnostics::Error("GPU mesh preparation received an invalid source handle.");

	GpuMeshPreparedData output;
	output.Source = source;
	const MeshData& meshData = source.GetResource()->GetMeshData();
	if (!meshData.IsValid())
		throw Diagnostics::Error("GPU mesh preparation received invalid mesh geometry.");

	BuildBoundsAndRayTracing(output);
	BuildSkinInfluences(output);
	BuildMorphTargets(output);
	if (output.RayTracingVertices.empty() || output.RayTracingIndices.size() < 3u
	    || output.GpuSkinInfluences.size() != output.RayTracingVertices.size())
		throw Diagnostics::Error("GPU mesh preparation produced incomplete geometry.");
	return output;
}

void GpuMeshPreparation::BuildBoundsAndRayTracing(GpuMeshPreparedData& output)
{
	const MeshData& meshData = output.Source.GetResource()->GetMeshData();
	output.RayTracingVertices.reserve(meshData.vertices.size());
	for (const VertexData& vertex : meshData.vertices)
	{
		if (!output.HasLocalBounds)
		{
			output.LocalBoundsMin = vertex.position;
			output.LocalBoundsMax = vertex.position;
			output.HasLocalBounds = true;
		}
		else
		{
			output.LocalBoundsMin.x = (std::min) (output.LocalBoundsMin.x, vertex.position.x);
			output.LocalBoundsMin.y = (std::min) (output.LocalBoundsMin.y, vertex.position.y);
			output.LocalBoundsMin.z = (std::min) (output.LocalBoundsMin.z, vertex.position.z);
			output.LocalBoundsMax.x = (std::max) (output.LocalBoundsMax.x, vertex.position.x);
			output.LocalBoundsMax.y = (std::max) (output.LocalBoundsMax.y, vertex.position.y);
			output.LocalBoundsMax.z = (std::max) (output.LocalBoundsMax.z, vertex.position.z);
		}

		output.RayTracingVertices.push_back(
		    RayTracingHitVertex{.Position = vertex.position, .Normal = vertex.normal, .Tangent = vertex.tangent, .TexCoord0 = vertex.uv});
	}

	output.RayTracingIndices.assign(meshData.indices.begin(), meshData.indices.end());
}

void GpuMeshPreparation::BuildSkinInfluences(GpuMeshPreparedData& output)
{
	const std::size_t vertexCount = output.RayTracingVertices.size();
	output.GpuSkinInfluences.resize(vertexCount);

	const auto* skeletalMesh = dynamic_cast<const SkeletalCookedMesh*>(output.Source.GetResource().get());
	if (skeletalMesh == nullptr)
	{
		return;
	}

	const std::vector<VertexSkinInfluence>& influences = skeletalMesh->GetSkeletalMeshData().skinInfluences;
	if (influences.size() != vertexCount)
		throw Diagnostics::Error("Skeletal mesh skin influence count does not match its vertex count.");

	output.SkinInfluences = influences;
	for (std::size_t index = 0u; index < influences.size(); ++index)
	{
		output.GpuSkinInfluences[index] = ConvertSkinInfluence(influences[index]);
	}
}

void GpuMeshPreparation::BuildMorphTargets(GpuMeshPreparedData& output)
{
	const auto* skeletalMesh = dynamic_cast<const SkeletalCookedMesh*>(output.Source.GetResource().get());
	if (skeletalMesh == nullptr)
		return;

	const MeshMorphData& morphTargets = skeletalMesh->GetSkeletalMeshData().morphTargets;
	if (!morphTargets.HasTargets())
		return;

	const std::size_t vertexCount = output.RayTracingVertices.size();
	output.MorphTargetDeltas.reserve(vertexCount * morphTargets.targets.size());
	for (const MeshMorphTarget& target : morphTargets.targets)
	{
		if (!target.IsValidForVertexCount(vertexCount))
			throw Diagnostics::Error("Skeletal mesh morph target does not match its vertex count.");

		for (const MeshMorphTargetDelta& delta : target.deltas)
		{
			output.MorphTargetDeltas.push_back(ConvertMorphTargetDelta(delta));
		}
	}

	output.MorphTargetCount = static_cast<std::uint32_t>(morphTargets.targets.size());
}

VertexSkinInfluenceData GpuMeshPreparation::ConvertSkinInfluence(const VertexSkinInfluence& influence) noexcept
{
	return VertexSkinInfluenceData{
	    .JointIndices0 = {influence.jointIndices[0], influence.jointIndices[1], influence.jointIndices[2], influence.jointIndices[3]},
	    .JointIndices1 = {influence.jointIndices[4], influence.jointIndices[5], influence.jointIndices[6], influence.jointIndices[7]},
	    .JointWeights0 = {influence.jointWeights[0], influence.jointWeights[1], influence.jointWeights[2], influence.jointWeights[3]},
	    .JointWeights1 = {influence.jointWeights[4], influence.jointWeights[5], influence.jointWeights[6], influence.jointWeights[7]}};
}

MorphTargetDeltaData GpuMeshPreparation::ConvertMorphTargetDelta(const MeshMorphTargetDelta& delta) noexcept
{
	return MorphTargetDeltaData{
	    .Position = {delta.position.x, delta.position.y, delta.position.z, 0.0f},
	    .Normal = {delta.normal.x, delta.normal.y, delta.normal.z, 0.0f},
	    .Tangent = {delta.tangent.x, delta.tangent.y, delta.tangent.z, 0.0f}};
}
