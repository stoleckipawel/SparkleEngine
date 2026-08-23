#pragma once

#include "/Engine/Resources/MeshInstanceShaderData.hlsli"

struct MorphedVertexAttributes
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
};

bool IsMorphedMeshInstance(const MeshInstanceData meshInstance)
{
	return (meshInstance.Flags & MeshInstanceFlag_Morphed) != 0u;
}

MorphedVertexAttributes ApplyMorphing(const MeshInstanceData meshInstance,
                                      const uint morphTargetDeltaOffset,
                                      const uint vertexId,
                                      const float3 position,
                                      const float3 normal,
                                      const float3 tangent)
{
	MorphedVertexAttributes attributes;
	attributes.Position = position;
	attributes.Normal = normal;
	attributes.Tangent = tangent;
	if (!IsMorphedMeshInstance(meshInstance))
	{
		return attributes;
	}

	for (uint targetIndex = 0u; targetIndex < meshInstance.MorphTargetCount; ++targetIndex)
	{
		const float weight = MorphWeights[meshInstance.MorphWeightOffset + targetIndex];
		const uint deltaIndex = morphTargetDeltaOffset + targetIndex * meshInstance.MorphTargetVertexCount + vertexId;
		const MorphTargetDeltaData delta = MorphTargetDeltas[deltaIndex];
		attributes.Position += delta.Position.xyz * weight;
		attributes.Normal += delta.Normal.xyz * weight;
		attributes.Tangent += delta.Tangent.xyz * weight;
	}
	return attributes;
}

MorphedVertexAttributes ApplyPreviousMorphing(const MeshInstanceData meshInstance,
                                              const uint morphTargetDeltaOffset,
                                              const uint vertexId,
                                              const float3 position,
                                              const float3 normal,
                                              const float3 tangent)
{
	MorphedVertexAttributes attributes;
	attributes.Position = position;
	attributes.Normal = normal;
	attributes.Tangent = tangent;
	if (!IsMorphedMeshInstance(meshInstance))
	{
		return attributes;
	}

	for (uint targetIndex = 0u; targetIndex < meshInstance.MorphTargetCount; ++targetIndex)
	{
		const float weight = PreviousMorphWeights[meshInstance.MorphWeightOffset + targetIndex];
		const uint deltaIndex = morphTargetDeltaOffset + targetIndex * meshInstance.MorphTargetVertexCount + vertexId;
		const MorphTargetDeltaData delta = MorphTargetDeltas[deltaIndex];
		attributes.Position += delta.Position.xyz * weight;
		attributes.Normal += delta.Normal.xyz * weight;
		attributes.Tangent += delta.Tangent.xyz * weight;
	}
	return attributes;
}
