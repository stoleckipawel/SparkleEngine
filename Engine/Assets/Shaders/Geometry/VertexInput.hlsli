#pragma once






namespace VS
{
	struct Input
	{
		float3 Position : POSITION;
		float2 TexCoord : TEXCOORD;
		float4 Color : COLOR;
		float3 Normal : NORMAL;
		float4 Tangent : TANGENT;
	};
}
