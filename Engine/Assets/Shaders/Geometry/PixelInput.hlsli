#pragma once

namespace PS
{
	struct Input
	{
		float4 Position : SV_POSITION;
		float2 TexCoord : TEXCOORD0;
		float3 PositionWorld : TEXCOORD1;
		float4 Color : COLOR;
		float3 NormalWorld : NORMAL;
		float3 TangentWorld : TANGENT;
		float3 BitangentWorld : BINORMAL;
	};


	void PrepareInput(inout Input input)
	{
		input.NormalWorld = normalize(input.NormalWorld);
		input.TangentWorld = normalize(input.TangentWorld);
		input.BitangentWorld = normalize(input.BitangentWorld);
	}
}  // namespace PS
