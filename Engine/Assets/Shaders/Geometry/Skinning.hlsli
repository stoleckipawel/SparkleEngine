#pragma once

struct SkinnedVertexAttributes
{
	float3 Position;
	float3 Normal;
	float3 Tangent;
};

float3 TransformSkinnedNormal(const float3 normal, const float4x4 skinningMatrix)
{
	const float3x3 linearTransform = (float3x3) skinningMatrix;
	const float3x3 cofactorMatrix = float3x3(
	    cross(linearTransform[1], linearTransform[2]),
	    cross(linearTransform[2], linearTransform[0]),
	    cross(linearTransform[0], linearTransform[1]));
	const float determinant = dot(linearTransform[0], cofactorMatrix[0]);
	if (abs(determinant) <= 1.0e-8f)
	{
		return normalize(mul(normal, linearTransform));
	}
	return normalize(mul(normal, cofactorMatrix) * (determinant < 0.0f ? -1.0f : 1.0f));
}

bool IsSkinnedMeshInstance(const MeshInstanceData meshInstance)
{
	return (meshInstance.Flags & MeshInstanceFlag_Skinned) != 0u &&
	       meshInstance.JointMatrixOffset != InvalidMeshInstanceJointMatrixOffset;
}

float4x4 LoadSkinningMatrix(const MeshInstanceData meshInstance, const uint vertexIndex)
{
	const VertexSkinInfluenceData skinInfluence = SkinInfluences[vertexIndex];
	return JointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices0.x].Matrix * skinInfluence.JointWeights0.x +
	       JointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices0.y].Matrix * skinInfluence.JointWeights0.y +
	       JointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices0.z].Matrix * skinInfluence.JointWeights0.z +
	       JointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices0.w].Matrix * skinInfluence.JointWeights0.w +
	       JointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices1.x].Matrix * skinInfluence.JointWeights1.x +
	       JointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices1.y].Matrix * skinInfluence.JointWeights1.y +
	       JointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices1.z].Matrix * skinInfluence.JointWeights1.z +
	       JointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices1.w].Matrix * skinInfluence.JointWeights1.w;
}

float4x4 LoadPreviousSkinningMatrix(const MeshInstanceData meshInstance, const uint vertexIndex)
{
	const VertexSkinInfluenceData skinInfluence = SkinInfluences[vertexIndex];
	return PreviousJointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices0.x].Matrix * skinInfluence.JointWeights0.x +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices0.y].Matrix * skinInfluence.JointWeights0.y +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices0.z].Matrix * skinInfluence.JointWeights0.z +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices0.w].Matrix * skinInfluence.JointWeights0.w +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices1.x].Matrix * skinInfluence.JointWeights1.x +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices1.y].Matrix * skinInfluence.JointWeights1.y +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices1.z].Matrix * skinInfluence.JointWeights1.z +
	       PreviousJointMatrices[meshInstance.JointMatrixOffset + skinInfluence.JointIndices1.w].Matrix * skinInfluence.JointWeights1.w;
}

SkinnedVertexAttributes ApplySkinning(
    const MeshInstanceData meshInstance,
    const uint vertexIndex,
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

	const float4x4 skinningMatrix = LoadSkinningMatrix(meshInstance, vertexIndex);
	attributes.Position = mul(float4(position, 1.0f), skinningMatrix).xyz;
	attributes.Normal = TransformSkinnedNormal(normal, skinningMatrix);
	attributes.Tangent = normalize(mul(tangent, (float3x3) skinningMatrix));
	return attributes;
}

SkinnedVertexAttributes ApplyPreviousSkinning(
    const MeshInstanceData meshInstance,
    const uint vertexIndex,
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

	const float4x4 skinningMatrix = LoadPreviousSkinningMatrix(meshInstance, vertexIndex);
	attributes.Position = mul(float4(position, 1.0f), skinningMatrix).xyz;
	attributes.Normal = TransformSkinnedNormal(normal, skinningMatrix);
	attributes.Tangent = normalize(mul(tangent, (float3x3) skinningMatrix));
	return attributes;
}
