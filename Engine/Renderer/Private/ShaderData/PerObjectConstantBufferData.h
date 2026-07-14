#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

struct alignas(256) PerObjectVSConstantBufferData
{
	DirectX::XMFLOAT4X4 WorldMTX;
	DirectX::XMFLOAT3X4 WorldInvTransposeMTX;
};
static_assert(std::is_standard_layout_v<PerObjectVSConstantBufferData>);
static_assert(std::is_trivially_copyable_v<PerObjectVSConstantBufferData>);
static_assert(alignof(PerObjectVSConstantBufferData) >= 256);
static_assert(sizeof(PerObjectVSConstantBufferData) % 256 == 0);
static_assert(sizeof(PerObjectVSConstantBufferData) <= 64 * 1024);

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
static_assert(std::is_standard_layout_v<PerObjectPSConstantBufferData>);
static_assert(std::is_trivially_copyable_v<PerObjectPSConstantBufferData>);
static_assert(alignof(PerObjectPSConstantBufferData) >= 256);
static_assert(sizeof(PerObjectPSConstantBufferData) % 256 == 0);
static_assert(sizeof(PerObjectPSConstantBufferData) <= 64 * 1024);
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
