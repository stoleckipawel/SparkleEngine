#pragma once

cbuffer PerObjectVSConstantBufferData
{
	row_major float4x4 WorldMatrix;
	row_major float3x3 WorldInverseTranspose;
};

cbuffer PerObjectPS
{
	float4 BaseColor;

	float3 EmissiveColor;
	float Metallic;

	float Roughness;
	float F0;
	float AlphaCutoff;
	uint AlphaMode;

	uint TextureFlags;
	float3 SubsurfaceColor;

	float SubsurfaceStrength;
	float3 _padPerObjectPS0;
};
