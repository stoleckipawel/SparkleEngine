#pragma once

namespace VS
{
	struct Output
	{
		float4 Position : SV_POSITION;
		float2 TexCoord : TEXCOORD0;
		float3 NormalWorld : TEXCOORD2;
		float4 TangentWorld : TEXCOORD3;
		float3 BitangentWorld : TEXCOORD4;
		float4 PrevClipPosition : TEXCOORD7;
		nointerpolation uint GpuSceneSlot : TEXCOORD8;
	};
}
