#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <type_traits>

struct MeshInstanceData
{
	DirectX::XMFLOAT4X4 WorldMatrix;
	DirectX::XMFLOAT4X4 PreviousWorldMatrix;
	DirectX::XMFLOAT3X4 WorldInverseTranspose;
	uint32_t MaterialSlot = 0;
	uint32_t Flags = 0;
	uint32_t JointMatrixOffset = 0;
	uint32_t MorphWeightOffset = 0;
	uint32_t MorphTargetCount = 0;
	uint32_t MorphTargetVertexCount = 0;
	uint32_t GpuSceneSlot = 0;
	uint32_t Reserved = 0;
};
static_assert(std::is_standard_layout_v<MeshInstanceData>, "MeshInstanceData must be standard-layout");
static_assert(std::is_trivially_copyable_v<MeshInstanceData>, "MeshInstanceData must be trivially-copyable");
static_assert(sizeof(MeshInstanceData) == 208, "MeshInstanceData must match the HLSL structured-buffer stride");

inline constexpr std::uint32_t MeshInstanceFlag_Skinned = 1u << 0u;
inline constexpr std::uint32_t MeshInstanceFlag_Morphed = 1u << 1u;
inline constexpr std::uint32_t kInvalidMeshInstanceJointMatrixOffset = (std::numeric_limits<std::uint32_t>::max)();
inline constexpr std::uint32_t kInvalidMeshInstanceMorphWeightOffset = (std::numeric_limits<std::uint32_t>::max)();

struct VertexSkinInfluenceData
{
	DirectX::XMUINT4 JointIndices0 = {0, 0, 0, 0};
	DirectX::XMUINT4 JointIndices1 = {0, 0, 0, 0};
	DirectX::XMFLOAT4 JointWeights0 = {0.0f, 0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4 JointWeights1 = {0.0f, 0.0f, 0.0f, 0.0f};
};
static_assert(std::is_standard_layout_v<VertexSkinInfluenceData>, "VertexSkinInfluenceData must be standard-layout");
static_assert(std::is_trivially_copyable_v<VertexSkinInfluenceData>, "VertexSkinInfluenceData must be trivially-copyable");
static_assert(sizeof(VertexSkinInfluenceData) == 64, "VertexSkinInfluenceData must match the HLSL structured-buffer stride");

struct JointMatrixData
{
	DirectX::XMFLOAT4X4 Matrix;
};
static_assert(std::is_standard_layout_v<JointMatrixData>, "JointMatrixData must be standard-layout");
static_assert(std::is_trivially_copyable_v<JointMatrixData>, "JointMatrixData must be trivially-copyable");
static_assert(sizeof(JointMatrixData) == 64, "JointMatrixData must match the HLSL structured-buffer stride");

struct alignas(256) MeshInstanceDrawConstantBufferData
{
	uint32_t FirstInstance = 0;
	DirectX::XMUINT3 Padding = {0, 0, 0};
};
static_assert(std::is_standard_layout_v<MeshInstanceDrawConstantBufferData>);
static_assert(std::is_trivially_copyable_v<MeshInstanceDrawConstantBufferData>);
static_assert(alignof(MeshInstanceDrawConstantBufferData) >= 256);
static_assert(sizeof(MeshInstanceDrawConstantBufferData) % 256 == 0);
static_assert(sizeof(MeshInstanceDrawConstantBufferData) <= 64 * 1024);
