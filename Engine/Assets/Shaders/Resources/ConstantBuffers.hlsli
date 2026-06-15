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

cbuffer PerTemporalConstantBufferData
{
	row_major float4x4 PrevViewMTX;
	row_major float4x4 PrevProjectionMTX;
	row_major float4x4 PrevViewProjMTX;
	float2 JitterCurrent;
	float2 JitterPrevious;
	uint HistoryValid;
	float4 _pad0;
	float4 _pad1;
};

cbuffer PerObjectVSConstantBufferData
{
	row_major float4x4 WorldMTX;
	row_major float3x3 WorldInvTransposeMTX;
};

struct MeshInstanceData
{
	row_major float4x4 WorldMTX;
	row_major float4x4 PreviousWorldMTX;
	row_major float3x4 WorldInvTransposeMTX;
	uint MaterialSlot;
	uint Flags;
	uint JointMatrixOffset;
	uint PackedDebugData;
};

static const uint MeshInstanceFlag_Skinned = 1u << 0u;
static const uint InvalidMeshInstanceJointMatrixOffset = 0xFFFFFFFFu;

struct VertexSkinInfluenceData
{
	uint4 JointIndices;
	float4 JointWeights;
};

struct JointMatrixData
{
	row_major float4x4 SkinningMTX;
};

cbuffer MeshInstanceDrawConstantBufferData
{
	uint FirstInstance;
	uint3 Padding;
};

StructuredBuffer<MeshInstanceData> MeshInstances;
StructuredBuffer<VertexSkinInfluenceData> SkinInfluences;
StructuredBuffer<JointMatrixData> JointMatrices;
StructuredBuffer<JointMatrixData> PreviousJointMatrices;

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
