#include "CommonVS.hlsli"
#include "Geometry/Skinning.hlsli"

void main(in VS::Input Input, out VS::Output Output)
{
	const uint instanceId = FirstInstance + Input.InstanceId;
	const MeshInstanceData meshInstance = MeshInstances[instanceId];
	const float4x4 worldMatrix = meshInstance.WorldMTX;
	const float4x4 previousWorldMatrix = meshInstance.PreviousWorldMTX;
	const float3x3 worldInvTransposeMatrix = (float3x3) meshInstance.WorldInvTransposeMTX;

	const SkinnedVertexAttributes localVertex = ApplySkinning(meshInstance, Input.VertexId, Input.Position, Input.Normal, Input.Tangent.xyz);

	const float4 positionWorld = mul(float4(localVertex.Position, 1.0f), worldMatrix);
	const float4 previousPositionWorld = mul(float4(localVertex.Position, 1.0f), previousWorldMatrix);
	const float3 normalWorld = normalize(mul(localVertex.Normal, worldInvTransposeMatrix));
	const float4 tangentWorld = float4(mul(localVertex.Tangent, (float3x3) worldMatrix), Input.Tangent.w);

	const float3 bitangentWorld = ComputeBitangent(normalWorld, tangentWorld);

	const float4 positionClip = PositionWorldToClip(positionWorld);
	const float4 previousClipPosition = mul(previousPositionWorld, PrevViewProjMTX);
	const float4 jitteredPositionClip = ApplyTemporalJitterClipOffset(positionClip, JitterCurrent);

	// Rasterize jittered samples for DLSS/DLAA, but keep motion-vector inputs unjittered.
	Output.Position = jitteredPositionClip;
	Output.PositionWorld = positionWorld.xyz;
	Output.NormalWorld = normalWorld;
	Output.TangentWorld = tangentWorld;
	Output.BitangentWorld = bitangentWorld;
	Output.TexCoord = Input.TexCoord;
	Output.Color = Input.Color;
	Output.InstanceId = instanceId;
	Output.ClipPosition = positionClip;
	Output.PrevClipPosition = previousClipPosition;
	Output.PackedDebugData = meshInstance.PackedDebugData;
}
