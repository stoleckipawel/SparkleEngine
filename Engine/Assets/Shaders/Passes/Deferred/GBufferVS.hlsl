#include "CommonVS.hlsli"
#include "Resources/TemporalConstantBuffer.hlsli"
#include "Geometry/Basis.hlsli"
#include "Geometry/Morphing.hlsli"
#include "Geometry/Skinning.hlsli"

void main(in VS::Input Input, out VS::Output Output)
{
	const uint packedInstanceId = FirstInstance + Input.InstanceId;
	const uint instanceSlot = MeshInstanceSlots[packedInstanceId];
	const MeshInstanceData meshInstance = MeshInstances[instanceSlot];
	const float4x4 worldMatrix = meshInstance.WorldMTX;
	const float4x4 previousWorldMatrix = meshInstance.PreviousWorldMTX;
	const float3x3 worldInvTransposeMatrix = (float3x3) meshInstance.WorldInvTransposeMTX;

	const MorphedVertexAttributes morphedVertex =
	    ApplyMorphing(
	        meshInstance,
	        0u,
	        Input.VertexId,
	        Input.Position,
	        Input.Normal,
	        Input.Tangent.xyz);
	const MorphedVertexAttributes previousMorphedVertex =
	    ApplyPreviousMorphing(
	        meshInstance,
	        0u,
	        Input.VertexId,
	        Input.Position,
	        Input.Normal,
	        Input.Tangent.xyz);
	const SkinnedVertexAttributes localVertex =
	    ApplySkinning(
	        meshInstance,
	        Input.VertexId,
	        morphedVertex.Position,
	        morphedVertex.Normal,
	        morphedVertex.Tangent);
	const SkinnedVertexAttributes previousLocalVertex =
	    ApplyPreviousSkinning(
	        meshInstance,
	        Input.VertexId,
	        previousMorphedVertex.Position,
	        previousMorphedVertex.Normal,
	        previousMorphedVertex.Tangent);

	const float4 positionWorld = mul(float4(localVertex.Position, 1.0f), worldMatrix);
	const float4 previousPositionWorld = mul(float4(previousLocalVertex.Position, 1.0f), previousWorldMatrix);
	const float3 normalWorld = normalize(mul(localVertex.Normal, worldInvTransposeMatrix));
	const float3 tangentWorld = OrthonormalizeTangent(mul(localVertex.Tangent, (float3x3) worldMatrix), normalWorld);
	const float3 bitangentWorld = ComputeBitangentFromSign(normalWorld, tangentWorld, Input.Tangent.w);

	const float4 positionClip = PositionWorldToClip(positionWorld);
	const float4 previousClipPosition = mul(previousPositionWorld, PrevViewProjMTX);
	const float4 jitteredPositionClip = ApplyTemporalJitterClipOffset(positionClip, JitterCurrent);

	// Rasterize jittered samples for DLSS/DLAA, but keep motion-vector inputs unjittered.
	Output.Position = jitteredPositionClip;
	Output.NormalWorld = normalWorld;
	Output.TangentWorld = float4(tangentWorld, Input.Tangent.w);
	Output.BitangentWorld = bitangentWorld;
	Output.TexCoord = Input.TexCoord;
	Output.PrevClipPosition = previousClipPosition;
	Output.DebugData = meshInstance.DebugData;
}
