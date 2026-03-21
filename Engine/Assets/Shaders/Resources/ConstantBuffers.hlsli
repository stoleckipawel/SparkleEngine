#pragma once

#include "Resources/CameraConstantBufferData.hlsli"
#include "Resources/LightConstantBufferData.hlsli"









cbuffer PerFrameConstantBufferData : register(b0)
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




cbuffer PerViewConstantBufferData : register(b1)
{
	row_major float4x4 ViewMTX;
	row_major float4x4 ProjectionMTX;
	row_major float4x4 ViewProjMTX;

	PerViewCameraConstantBufferData Camera;
	PerViewLightingConstantBufferData ViewLighting;
};




cbuffer PerObjectVSConstantBufferData : register(b2)
{
	row_major float4x4 WorldMTX;
	row_major float3x3 WorldInvTransposeMTX;


};




cbuffer PerObjectPSConstantBufferData : register(b3)
{
	float4 BaseColor;

	float3 EmissiveColor;
	float Metallic;

	float Roughness;
	float F0;
	float AlphaCutoff;
	uint AlphaMode;

	uint TextureFlags;
	float3 _padPerObjectPS0;


};
