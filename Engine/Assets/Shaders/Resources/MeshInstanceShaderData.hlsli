#pragma once

struct MeshInstanceData
{
	row_major float4x4 WorldMatrix;
	row_major float4x4 PreviousWorldMatrix;
	row_major float3x4 WorldInverseTranspose;
	uint MaterialSlot;
	uint Flags;
	uint JointMatrixOffset;
	uint MorphWeightOffset;
	uint MorphTargetCount;
	uint MorphTargetVertexCount;
	uint GpuSceneSlot;
	uint Reserved;
};

static const uint MeshInstanceFlag_Skinned = 1u << 0u;
static const uint MeshInstanceFlag_Morphed = 1u << 1u;
static const uint InvalidMeshInstanceJointMatrixOffset = 0xFFFFFFFFu;

struct MorphTargetDeltaData
{
	float4 Position;
	float4 Normal;
	float4 Tangent;
};

struct VertexSkinInfluenceData
{
	uint4 JointIndices0;
	uint4 JointIndices1;
	float4 JointWeights0;
	float4 JointWeights1;
};

struct JointMatrixData
{
	row_major float4x4 Matrix;
};

cbuffer MeshInstanceDraw
{
	uint FirstInstance;
	uint3 Padding;
};

StructuredBuffer<MeshInstanceData> MeshInstances;
StructuredBuffer<uint> MeshInstanceSlots;
StructuredBuffer<VertexSkinInfluenceData> SkinInfluences;
StructuredBuffer<JointMatrixData> JointMatrices;
StructuredBuffer<JointMatrixData> PreviousJointMatrices;
StructuredBuffer<MorphTargetDeltaData> MorphTargetDeltas;
StructuredBuffer<float> MorphWeights;
StructuredBuffer<float> PreviousMorphWeights;
