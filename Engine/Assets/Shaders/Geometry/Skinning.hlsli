#pragma once

struct SkinnedVertexAttributes
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
};

bool IsSkinnedMeshInstance(const MeshInstanceData meshInstance)
{
	return (meshInstance.Flags & MeshInstanceFlag_Skinned) != 0u &&
	       meshInstance.JointMatrixOffset != InvalidMeshInstanceJointMatrixOffset;
}

float4x4 LoadSkinningMatrix(const MeshInstanceData meshInstance, const uint vertexId)
{
	const VertexSkinInfluenceData skin = SkinInfluences[vertexId];
	return JointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.x].SkinningMTX * skin.JointWeights.x +
	       JointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.y].SkinningMTX * skin.JointWeights.y +
	       JointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.z].SkinningMTX * skin.JointWeights.z +
	       JointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.w].SkinningMTX * skin.JointWeights.w;
}

float4x4 LoadPreviousSkinningMatrix(const MeshInstanceData meshInstance, const uint vertexId)
{
	const VertexSkinInfluenceData skin = SkinInfluences[vertexId];
	return PreviousJointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.x].SkinningMTX * skin.JointWeights.x +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.y].SkinningMTX * skin.JointWeights.y +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.z].SkinningMTX * skin.JointWeights.z +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.w].SkinningMTX * skin.JointWeights.w;
}

SkinnedVertexAttributes ApplySkinning(
    const MeshInstanceData meshInstance,
    const uint vertexId,
    const float3 position,
    const float3 normal,
    const float3 tangent)
{
	SkinnedVertexAttributes attributes;
	attributes.Position = position;
	attributes.Normal = normal;
	attributes.Tangent = tangent;

	if (!IsSkinnedMeshInstance(meshInstance))
	{
		return attributes;
	}

	const float4x4 skinningMatrix = LoadSkinningMatrix(meshInstance, vertexId);
	attributes.Position = mul(float4(position, 1.0f), skinningMatrix).xyz;
	attributes.Normal = normalize(mul(normal, (float3x3) skinningMatrix));
	attributes.Tangent = normalize(mul(tangent, (float3x3) skinningMatrix));
	return attributes;
}

SkinnedVertexAttributes ApplyPreviousSkinning(
    const MeshInstanceData meshInstance,
    const uint vertexId,
    const float3 position,
    const float3 normal,
    const float3 tangent)
{
	SkinnedVertexAttributes attributes;
	attributes.Position = position;
	attributes.Normal = normal;
	attributes.Tangent = tangent;

	if (!IsSkinnedMeshInstance(meshInstance))
	{
		return attributes;
	}

	const float4x4 skinningMatrix = LoadPreviousSkinningMatrix(meshInstance, vertexId);
	attributes.Position = mul(float4(position, 1.0f), skinningMatrix).xyz;
	attributes.Normal = normalize(mul(normal, (float3x3) skinningMatrix));
	attributes.Tangent = normalize(mul(tangent, (float3x3) skinningMatrix));
	return attributes;
}
