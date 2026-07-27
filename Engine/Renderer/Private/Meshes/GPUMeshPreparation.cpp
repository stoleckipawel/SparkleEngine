#include "PCH.h"
#include "Meshes/GPUMeshPreparation.h"

#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshData.h"
#include "Scene/Meshes/MeshMorphData.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"

#include <algorithm>

bool GPUMeshPreparedData::IsValid() const noexcept
{
	return Source.IsValid() &&
	       !RayTracingVertices.empty() &&
	       RayTracingIndices.size() >= 3u &&
	       GpuSkinInfluences.size() ==
	           RayTracingVertices.size();
}

std::uint64_t GPUMeshPreparedData::GetDecodedByteSize() const noexcept
{
	return RayTracingVertices.size() * sizeof(RayTracingHitVertex) +
	       RayTracingIndices.size() * sizeof(std::uint32_t) +
	       SkinInfluences.size() * sizeof(VertexSkinInfluence) +
	       GpuSkinInfluences.size() * sizeof(VertexSkinInfluenceData) +
	       MorphTargetDeltas.size() * sizeof(MorphTargetDeltaData);
}

std::uint64_t GPUMeshPreparedData::GetResidentByteSize() const noexcept
{
	if (!Source.IsValid())
	{
		return 0;
	}

	const MeshData& meshData = Source.GetResource()->GetMeshData();
	return GetDecodedByteSize() +
	       meshData.GetVertexBufferSize() +
	       meshData.GetIndexBufferSize();
}

bool GPUMeshPreparation::Build(
    const ImmutableRenderMeshHandle& source,
    GPUMeshPreparedData& output)
{
	output = {};
	if (!source.IsValid())
	{
		return false;
	}

	output.Source = source;
	const MeshData& meshData =
	    source.GetResource()->GetMeshData();
	if (!meshData.IsValid())
	{
		return false;
	}

	BuildBoundsAndRayTracing(output);
	BuildSkinInfluences(output);
	if (!BuildMorphTargets(output))
	{
		output = {};
		return false;
	}

	return output.IsValid();
}

void GPUMeshPreparation::BuildBoundsAndRayTracing(
    GPUMeshPreparedData& output)
{
	const MeshData& meshData =
	    output.Source.GetResource()->GetMeshData();
	output.RayTracingVertices.reserve(
	    meshData.vertices.size());
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
			output.LocalBoundsMin.x =
			    (std::min)(
			        output.LocalBoundsMin.x,
			        vertex.position.x);
			output.LocalBoundsMin.y =
			    (std::min)(
			        output.LocalBoundsMin.y,
			        vertex.position.y);
			output.LocalBoundsMin.z =
			    (std::min)(
			        output.LocalBoundsMin.z,
			        vertex.position.z);
			output.LocalBoundsMax.x =
			    (std::max)(
			        output.LocalBoundsMax.x,
			        vertex.position.x);
			output.LocalBoundsMax.y =
			    (std::max)(
			        output.LocalBoundsMax.y,
			        vertex.position.y);
			output.LocalBoundsMax.z =
			    (std::max)(
			        output.LocalBoundsMax.z,
			        vertex.position.z);
		}

		output.RayTracingVertices.push_back(
		    RayTracingHitVertex{
		        .Position = vertex.position,
		        .Normal = vertex.normal,
		        .Tangent = vertex.tangent,
		        .TexCoord0 = vertex.uv});
	}

	output.RayTracingIndices.assign(
	    meshData.indices.begin(),
	    meshData.indices.end());
}

void GPUMeshPreparation::BuildSkinInfluences(
    GPUMeshPreparedData& output)
{
	const std::size_t vertexCount =
	    output.RayTracingVertices.size();
	output.GpuSkinInfluences.resize(vertexCount);

	const auto* skeletalMesh =
	    dynamic_cast<const SkeletalCookedMesh*>(
	        output.Source.GetResource().get());
	if (skeletalMesh == nullptr)
	{
		return;
	}

	const std::vector<VertexSkinInfluence>& influences =
	    skeletalMesh->GetSkeletalMeshData()
	        .skinInfluences;
	if (influences.size() != vertexCount)
	{
		return;
	}

	output.SkinInfluences = influences;
	for (std::size_t index = 0u;
	     index < influences.size();
	     ++index)
	{
		output.GpuSkinInfluences[index] =
		    ConvertSkinInfluence(influences[index]);
	}
}

bool GPUMeshPreparation::BuildMorphTargets(
    GPUMeshPreparedData& output)
{
	const auto* skeletalMesh =
	    dynamic_cast<const SkeletalCookedMesh*>(
	        output.Source.GetResource().get());
	if (skeletalMesh == nullptr)
	{
		return true;
	}

	const MeshMorphData& morphTargets =
	    skeletalMesh->GetSkeletalMeshData()
	        .morphTargets;
	if (!morphTargets.HasTargets())
	{
		return true;
	}

	const std::size_t vertexCount =
	    output.RayTracingVertices.size();
	output.MorphTargetDeltas.reserve(
	    vertexCount * morphTargets.targets.size());
	for (const MeshMorphTarget& target :
	     morphTargets.targets)
	{
		if (!target.IsValidForVertexCount(vertexCount))
		{
			return false;
		}

		for (const MeshMorphTargetDelta& delta :
		     target.deltas)
		{
			output.MorphTargetDeltas.push_back(
			    ConvertMorphTargetDelta(delta));
		}
	}

	output.MorphTargetCount =
	    static_cast<std::uint32_t>(
	        morphTargets.targets.size());
	return true;
}

VertexSkinInfluenceData
GPUMeshPreparation::ConvertSkinInfluence(
    const VertexSkinInfluence& influence) noexcept
{
	return VertexSkinInfluenceData{
	    .JointIndices =
	        {influence.jointIndices[0],
	         influence.jointIndices[1],
	         influence.jointIndices[2],
	         influence.jointIndices[3]},
	    .JointWeights =
	        {influence.jointWeights[0],
	         influence.jointWeights[1],
	         influence.jointWeights[2],
	         influence.jointWeights[3]}};
}

MorphTargetDeltaData
GPUMeshPreparation::ConvertMorphTargetDelta(
    const MeshMorphTargetDelta& delta) noexcept
{
	return MorphTargetDeltaData{
	    .Position =
	        {delta.position.x,
	         delta.position.y,
	         delta.position.z,
	         0.0f},
	    .Normal =
	        {delta.normal.x,
	         delta.normal.y,
	         delta.normal.z,
	         0.0f},
	    .Tangent =
	        {delta.tangent.x,
	         delta.tangent.y,
	         delta.tangent.z,
	         0.0f}};
}
