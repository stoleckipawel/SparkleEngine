#include "CommonVS.hlsli"

void main(in VS::Input Input, out VS::Output Output)
{
	const MeshInstanceData meshInstance = MeshInstances[FirstInstance + Input.InstanceId];
	const float4x4 worldMatrix = meshInstance.WorldMTX;
	const float3x3 worldInvTransposeMatrix = (float3x3) meshInstance.WorldInvTransposeMTX;

	const float4 positionWorld = mul(float4(Input.Position, 1.0f), worldMatrix);
	const float3 normalWorld = normalize(mul(Input.Normal, worldInvTransposeMatrix));
	const float4 tangentWorld = float4(mul(Input.Tangent.xyz, (float3x3) worldMatrix), Input.Tangent.w);

	const float3 bitangentWorld = ComputeBitangent(normalWorld, tangentWorld);

	const float4 positionClip = PositionWorldToClip(positionWorld);

	Output.Position = positionClip;
	Output.PositionWorld = positionWorld.xyz;
	Output.NormalWorld = normalWorld;
	Output.TangentWorld = tangentWorld;
	Output.BitangentWorld = bitangentWorld;
	Output.TexCoord = Input.TexCoord;
	Output.Color = Input.Color;
}
