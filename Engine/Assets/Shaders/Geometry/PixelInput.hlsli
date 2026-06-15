#pragma once

namespace PS
{
	struct Input
	{
		float4 Position : SV_POSITION;
		float2 TexCoord : TEXCOORD0;
		float3 PositionWorld : TEXCOORD1;
		float4 Color : COLOR0;
		float3 NormalWorld : TEXCOORD2;
		float4 TangentWorld : TEXCOORD3;
		float3 BitangentWorld : TEXCOORD4;
		nointerpolation uint InstanceId : TEXCOORD5;
		float4 ClipPosition : TEXCOORD6;
		float4 PrevClipPosition : TEXCOORD7;
		nointerpolation uint PackedDebugData : TEXCOORD8;
		bool IsFrontFace : SV_IsFrontFace;
	};


	void PrepareInput(inout Input input)
	{
		input.NormalWorld = normalize(input.NormalWorld);
		input.TangentWorld = float4(normalize(input.TangentWorld.xyz), input.TangentWorld.w);
		input.BitangentWorld = normalize(input.BitangentWorld);
	}
}  // namespace PS
