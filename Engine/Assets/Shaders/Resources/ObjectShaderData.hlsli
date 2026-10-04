#pragma once

cbuffer PerObjectVSConstantBufferData
{
	row_major float4x4 WorldMatrix;
	// Match XMStoreFloat3x4: three columns, each occupying one 16-byte register.
	column_major float3x3 WorldInverseTranspose;
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
	float NormalScale;
	float2 _padPerObjectPS0;
};
