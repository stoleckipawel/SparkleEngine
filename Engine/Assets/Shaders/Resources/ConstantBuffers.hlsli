#pragma once

#include "Resources/CameraConstantBufferData.hlsli"
#include "Resources/LightConstantBufferData.hlsli"

cbuffer PerFrameConstantBufferData
{
	uint FrameIndex;
	float TotalTime;
	float DeltaTime;
	float ScaledTotalTime;
	float ScaledDeltaTime;
	uint ViewModeIndex;

	float2 ViewportSize;
	float2 ViewportSizeInv;
};

cbuffer PerViewConstantBufferData
{
	PerViewCameraConstantBufferData Camera;
	PerViewLightingConstantBufferData ViewLighting;
};

cbuffer PerObjectVSConstantBufferData
{
	row_major float4x4 WorldMTX;
	row_major float3x3 WorldInvTransposeMTX;
};

struct MeshInstanceData
{
	row_major float4x4 WorldMTX;
	row_major float3x4 WorldInvTransposeMTX;
	uint MaterialSlot;
	uint Flags;
	uint2 Padding;
};

cbuffer MeshInstanceDrawConstantBufferData
{
	uint FirstInstance;
	uint3 MeshInstanceDrawPadding;
};

StructuredBuffer<MeshInstanceData> MeshInstances;

cbuffer PerObjectPSConstantBufferData
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
