#pragma once
#include <cstddef>
#include <DirectXMath.h>
#include <cstdint>
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
	DirectX::XMFLOAT4X4 ViewMTX;
	DirectX::XMFLOAT4X4 ProjectionMTX;
	DirectX::XMFLOAT4X4 ViewProjMTX;

	DirectX::XMFLOAT3 CameraPosition;
	float NearZ;

	float FarZ;
	DirectX::XMFLOAT3 CameraDirection;

	DirectX::XMFLOAT3 SunDirection;
	float SunIntensity;

	DirectX::XMFLOAT3 SunColor;
	float _padPerView0;
};
CBV_CHECK(PerViewConstantBufferData);

struct alignas(256) PerObjectVSConstantBufferData
{
	DirectX::XMFLOAT4X4 WorldMTX;
	DirectX::XMFLOAT3X4 WorldInvTransposeMTX;
};
CBV_CHECK(PerObjectVSConstantBufferData);

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
