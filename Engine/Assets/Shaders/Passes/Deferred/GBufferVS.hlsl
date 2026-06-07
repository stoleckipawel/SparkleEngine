#include "CommonVS.hlsli"

void main(in VS::Input Input, out VS::Output Output)
{
	const uint instanceId = FirstInstance + Input.InstanceId;
	const MeshInstanceData meshInstance = MeshInstances[instanceId];
	const float4x4 worldMatrix = meshInstance.WorldMTX;
	const float3x3 worldInvTransposeMatrix = (float3x3) meshInstance.WorldInvTransposeMTX;

	float3 localPosition = Input.Position;
	float3 localNormal = Input.Normal;
	float3 localTangent = Input.Tangent.xyz;
	if ((meshInstance.Flags & MeshInstanceFlag_Skinned) != 0u &&
	    meshInstance.JointMatrixOffset != InvalidMeshInstanceJointMatrixOffset)
	{
		const VertexSkinInfluenceData skin = SkinInfluences[Input.VertexId];
		float4x4 skinMatrix =
		    JointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.x].SkinningMTX * skin.JointWeights.x +
		    JointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.y].SkinningMTX * skin.JointWeights.y +
		    JointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.z].SkinningMTX * skin.JointWeights.z +
		    JointMatrices[meshInstance.JointMatrixOffset + skin.JointIndices.w].SkinningMTX * skin.JointWeights.w;
		localPosition = mul(float4(localPosition, 1.0f), skinMatrix).xyz;
		localNormal = normalize(mul(localNormal, (float3x3) skinMatrix));
		localTangent = normalize(mul(localTangent, (float3x3) skinMatrix));
	}

	const float4 positionWorld = mul(float4(localPosition, 1.0f), worldMatrix);
	const float3 normalWorld = normalize(mul(localNormal, worldInvTransposeMatrix));
	const float4 tangentWorld = float4(mul(localTangent, (float3x3) worldMatrix), Input.Tangent.w);

	const float3 bitangentWorld = ComputeBitangent(normalWorld, tangentWorld);

	const float4 positionClip = PositionWorldToClip(positionWorld);

	Output.Position = positionClip;
	Output.PositionWorld = positionWorld.xyz;
	Output.NormalWorld = normalWorld;
	Output.TangentWorld = tangentWorld;
	Output.BitangentWorld = bitangentWorld;
	Output.TexCoord = Input.TexCoord;
	Output.Color = Input.Color;
	Output.InstanceId = instanceId;
}
