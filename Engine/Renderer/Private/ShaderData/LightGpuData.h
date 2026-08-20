#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>

struct DirectionalLightGpuData
{
	DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
	// Illuminance in lux.
	float Illuminance = 1.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};

	float AngularSizeRadians = 0.009308f;

	std::uint32_t CastShadow = 1u;
	DirectX::XMUINT3 Padding = {0u, 0u, 0u};
};

struct PointLightGpuData
{
	DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
	float Range = 0.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};
	// Luminous intensity in candela.
	float LuminousIntensity = 1.0f;

	DirectX::XMFLOAT3 DistanceAttenuationCoefficients = {0.0f, 0.0f, 1.0f};
	float Radius = 0.05f;

	std::uint32_t CastShadow = 1u;
	DirectX::XMUINT3 Padding = {0u, 0u, 0u};
};

struct SpotLightGpuData
{
	DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
	float Range = 0.0f;

	DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
	float InnerAngleCosine = 1.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};
	// Luminous intensity in candela.
	float LuminousIntensity = 1.0f;

	DirectX::XMFLOAT3 DistanceAttenuationCoefficients = {0.0f, 0.0f, 1.0f};
	float Radius = 0.05f;

	float OuterAngleCosine = 0.0f;
	std::uint32_t CastShadow = 1u;
	DirectX::XMUINT2 Padding = {0u, 0u};
};

struct RectLightGpuData
{
	DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
	float Width = 1.0f;

	DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
	float Height = 1.0f;

	DirectX::XMFLOAT3 Tangent = {1.0f, 0.0f, 0.0f};
	// Luminance in candela per square meter.
	float Luminance = 1.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};
	std::uint32_t CastShadow = 1u;
};

static_assert(sizeof(DirectionalLightGpuData) == 48, "Directional light GPU data must be 48 bytes");
static_assert(offsetof(DirectionalLightGpuData, Direction) == 0, "DirectionalLightGpuData::Direction must start at c0.xyz");
static_assert(offsetof(DirectionalLightGpuData, Illuminance) == 12, "DirectionalLightGpuData::Illuminance must be at c0.w");
static_assert(offsetof(DirectionalLightGpuData, Color) == 16, "DirectionalLightGpuData::Color must start at c1.xyz");
static_assert(offsetof(DirectionalLightGpuData, AngularSizeRadians) == 28, "DirectionalLightGpuData::AngularSizeRadians must be at c1.w");
static_assert(offsetof(DirectionalLightGpuData, CastShadow) == 32, "DirectionalLightGpuData::CastShadow must start at c2.x");
static_assert(sizeof(PointLightGpuData) == 64, "Point light GPU data must be 64 bytes");
static_assert(offsetof(PointLightGpuData, Position) == 0, "PointLightGpuData::Position must start at c0.xyz");
static_assert(offsetof(PointLightGpuData, Range) == 12, "PointLightGpuData::Range must be at c0.w");
static_assert(offsetof(PointLightGpuData, Color) == 16, "PointLightGpuData::Color must start at c1.xyz");
static_assert(offsetof(PointLightGpuData, LuminousIntensity) == 28, "PointLightGpuData::LuminousIntensity must be at c1.w");
static_assert(
    offsetof(PointLightGpuData, DistanceAttenuationCoefficients) == 32,
    "PointLightGpuData::DistanceAttenuationCoefficients must start at c2.xyz");
static_assert(offsetof(PointLightGpuData, Radius) == 44, "PointLightGpuData::Radius must be at c2.w");
static_assert(offsetof(PointLightGpuData, CastShadow) == 48, "PointLightGpuData::CastShadow must start at c3.x");
static_assert(sizeof(SpotLightGpuData) == 80, "Spot light GPU data must be 80 bytes");
static_assert(offsetof(SpotLightGpuData, Position) == 0, "SpotLightGpuData::Position must start at c0.xyz");
static_assert(offsetof(SpotLightGpuData, Range) == 12, "SpotLightGpuData::Range must be at c0.w");
static_assert(offsetof(SpotLightGpuData, Direction) == 16, "SpotLightGpuData::Direction must start at c1.xyz");
static_assert(offsetof(SpotLightGpuData, InnerAngleCosine) == 28, "SpotLightGpuData::InnerAngleCosine must be at c1.w");
static_assert(offsetof(SpotLightGpuData, Color) == 32, "SpotLightGpuData::Color must start at c2.xyz");
static_assert(offsetof(SpotLightGpuData, LuminousIntensity) == 44, "SpotLightGpuData::LuminousIntensity must be at c2.w");
static_assert(
    offsetof(SpotLightGpuData, DistanceAttenuationCoefficients) == 48,
    "SpotLightGpuData::DistanceAttenuationCoefficients must start at c3.xyz");
static_assert(offsetof(SpotLightGpuData, Radius) == 60, "SpotLightGpuData::Radius must be at c3.w");
static_assert(offsetof(SpotLightGpuData, OuterAngleCosine) == 64, "SpotLightGpuData::OuterAngleCosine must start at c4.x");
static_assert(offsetof(SpotLightGpuData, CastShadow) == 68, "SpotLightGpuData::CastShadow must be at c4.y");
static_assert(sizeof(RectLightGpuData) == 64, "Rect light GPU data must be 64 bytes");
static_assert(offsetof(RectLightGpuData, Position) == 0, "RectLightGpuData::Position must start at c0.xyz");
static_assert(offsetof(RectLightGpuData, Width) == 12, "RectLightGpuData::Width must be at c0.w");
static_assert(offsetof(RectLightGpuData, Direction) == 16, "RectLightGpuData::Direction must start at c1.xyz");
static_assert(offsetof(RectLightGpuData, Height) == 28, "RectLightGpuData::Height must be at c1.w");
static_assert(offsetof(RectLightGpuData, Tangent) == 32, "RectLightGpuData::Tangent must start at c2.xyz");
static_assert(offsetof(RectLightGpuData, Luminance) == 44, "RectLightGpuData::Luminance must be at c2.w");
static_assert(offsetof(RectLightGpuData, Color) == 48, "RectLightGpuData::Color must start at c3.xyz");
static_assert(offsetof(RectLightGpuData, CastShadow) == 60, "RectLightGpuData::CastShadow must be at c3.w");
