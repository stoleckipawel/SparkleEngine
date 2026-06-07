#pragma once

#include "RenderViewCameraData.h"
#include "RenderViewLightingData.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#define CBV_CHECK(Type)                                                                      \
	static_assert(std::is_standard_layout_v<Type>, #Type " must be standard-layout");        \
	static_assert(std::is_trivially_copyable_v<Type>, #Type " must be trivially-copyable");  \
	static_assert(alignof(Type) >= 256, #Type " must be 256-byte aligned");                  \
	static_assert(sizeof(Type) % 256 == 0, #Type " must occupy whole 256-byte CBV slot(s)"); \
	static_assert(sizeof(Type) <= 64 * 1024, #Type " must be <= 64KB")

struct alignas(256) PerFrameConstantBufferData
{
	uint32_t FrameIndex;
	float TotalTime;
	float DeltaTime;
	float ScaledTotalTime;
	float ScaledDeltaTime;
	uint32_t ViewModeIndex;

	DirectX::XMFLOAT2 ViewportSize;
	DirectX::XMFLOAT2 ViewportSizeInv;
};
CBV_CHECK(PerFrameConstantBufferData);

struct alignas(256) PerViewConstantBufferData
{
	PerViewCameraConstantBufferData Camera = {};
	PerViewLightingConstantBufferData ViewLighting = {};
};
CBV_CHECK(PerViewConstantBufferData);
static_assert(offsetof(PerViewConstantBufferData, Camera) == 0, "PerViewConstantBufferData::Camera must start at c0");
static_assert(
    offsetof(PerViewConstantBufferData, ViewLighting) == 352,
    "PerViewConstantBufferData::ViewLighting must start after camera data");
static_assert(sizeof(PerViewConstantBufferData) == 57856, "PerViewConstantBufferData must fit in aligned CBV slots");

struct alignas(256) PerObjectVSConstantBufferData
{
	DirectX::XMFLOAT4X4 WorldMTX;
	DirectX::XMFLOAT3X4 WorldInvTransposeMTX;
};
CBV_CHECK(PerObjectVSConstantBufferData);

struct MeshInstanceData
{
	DirectX::XMFLOAT4X4 WorldMTX;
	DirectX::XMFLOAT3X4 WorldInvTransposeMTX;
	uint32_t MaterialSlot = 0;
	uint32_t Flags = 0;
	uint32_t JointMatrixOffset = 0;
	uint32_t Padding = 0;
};
static_assert(std::is_standard_layout_v<MeshInstanceData>, "MeshInstanceData must be standard-layout");
static_assert(std::is_trivially_copyable_v<MeshInstanceData>, "MeshInstanceData must be trivially-copyable");
static_assert(sizeof(MeshInstanceData) == 128, "MeshInstanceData must match the HLSL structured-buffer stride");

inline constexpr std::uint32_t MeshInstanceFlag_Skinned = 1u << 0u;
inline constexpr std::uint32_t kInvalidMeshInstanceJointMatrixOffset = (std::numeric_limits<std::uint32_t>::max)();

struct VertexSkinInfluenceData
{
	DirectX::XMUINT4 JointIndices = {0, 0, 0, 0};
	DirectX::XMFLOAT4 JointWeights = {0.0f, 0.0f, 0.0f, 0.0f};
};
static_assert(std::is_standard_layout_v<VertexSkinInfluenceData>, "VertexSkinInfluenceData must be standard-layout");
static_assert(std::is_trivially_copyable_v<VertexSkinInfluenceData>, "VertexSkinInfluenceData must be trivially-copyable");
static_assert(sizeof(VertexSkinInfluenceData) == 32, "VertexSkinInfluenceData must match the HLSL structured-buffer stride");

struct JointMatrixData
{
	DirectX::XMFLOAT4X4 SkinningMTX;
};
static_assert(std::is_standard_layout_v<JointMatrixData>, "JointMatrixData must be standard-layout");
static_assert(std::is_trivially_copyable_v<JointMatrixData>, "JointMatrixData must be trivially-copyable");
static_assert(sizeof(JointMatrixData) == 64, "JointMatrixData must match the HLSL structured-buffer stride");

struct alignas(256) MeshInstanceDrawConstantBufferData
{
	uint32_t FirstInstance = 0;
	DirectX::XMUINT3 Padding = {0, 0, 0};
};
CBV_CHECK(MeshInstanceDrawConstantBufferData);

struct alignas(256) PerObjectPSConstantBufferData
{
	DirectX::XMFLOAT4 BaseColor;

	DirectX::XMFLOAT3 EmissiveColor;
	float Metallic;

	float Roughness;
	float F0;
	float AlphaCutoff;
	uint32_t AlphaMode;

	uint32_t TextureFlags;
	DirectX::XMFLOAT3 SubsurfaceColor = {0.0f, 0.0f, 0.0f};

	float SubsurfaceStrength = 0.0f;
	DirectX::XMFLOAT3 _padPerObjectPS0 = {0.0f, 0.0f, 0.0f};
};
CBV_CHECK(PerObjectPSConstantBufferData);
static_assert(offsetof(PerObjectPSConstantBufferData, BaseColor) == 0, "PerObjectPSConstantBufferData::BaseColor must start at c0");
static_assert(
    offsetof(PerObjectPSConstantBufferData, EmissiveColor) == 16,
    "PerObjectPSConstantBufferData::EmissiveColor must start at c1");
static_assert(offsetof(PerObjectPSConstantBufferData, Metallic) == 28, "PerObjectPSConstantBufferData::Metallic must share c1.w");
static_assert(offsetof(PerObjectPSConstantBufferData, Roughness) == 32, "PerObjectPSConstantBufferData::Roughness must start at c2.x");
static_assert(offsetof(PerObjectPSConstantBufferData, F0) == 36, "PerObjectPSConstantBufferData::F0 must start at c2.y");
static_assert(offsetof(PerObjectPSConstantBufferData, AlphaCutoff) == 40, "PerObjectPSConstantBufferData::AlphaCutoff must start at c2.z");
static_assert(offsetof(PerObjectPSConstantBufferData, AlphaMode) == 44, "PerObjectPSConstantBufferData::AlphaMode must start at c2.w");
static_assert(
    offsetof(PerObjectPSConstantBufferData, TextureFlags) == 48,
    "PerObjectPSConstantBufferData::TextureFlags must start at c3.x");
static_assert(
    offsetof(PerObjectPSConstantBufferData, SubsurfaceColor) == 52,
    "PerObjectPSConstantBufferData::SubsurfaceColor must start at c3.y");
static_assert(
    offsetof(PerObjectPSConstantBufferData, SubsurfaceStrength) == 64,
    "PerObjectPSConstantBufferData::SubsurfaceStrength must start at c4.x");
