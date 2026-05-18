#pragma once

namespace VS
{
	struct Output
	{
		float4 Position : SV_POSITION;
		float2 TexCoord : TEXCOORD0;
		float3 PositionWorld : TEXCOORD1;
		float4 Color : COLOR0;
		float3 NormalWorld : TEXCOORD2;
		float4 TangentWorld : TEXCOORD3;
		float3 BitangentWorld : TEXCOORD4;
		nointerpolation uint InstanceId : TEXCOORD5;
	};
}  // namespace VS
