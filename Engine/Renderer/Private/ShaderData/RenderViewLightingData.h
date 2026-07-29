#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>

struct DirectionalLightConstantBufferData
{
	DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
	// Illuminance in lux.
	float Illuminance = 1.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};

	float AngularSizeRadians = 0.009308f;

	std::uint32_t CastShadow = 1u;
	DirectX::XMUINT3 Padding = {0u, 0u, 0u};
};

struct PointLightConstantBufferData
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

struct SpotLightConstantBufferData
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

struct RectLightConstantBufferData
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

struct ViewLightingData
{
	std::uint32_t DirectionalLightCount = 0;
	std::uint32_t PointLightCount = 0;
	std::uint32_t SpotLightCount = 0;
	std::uint32_t RectLightCount = 0;
};

static_assert(sizeof(DirectionalLightConstantBufferData) == 48, "Directional light constant buffer data must be 48 bytes");
static_assert(
    offsetof(DirectionalLightConstantBufferData, Direction) == 0,
    "DirectionalLightConstantBufferData::Direction must start at c0.xyz");
static_assert(
    offsetof(DirectionalLightConstantBufferData, Illuminance) == 12,
    "DirectionalLightConstantBufferData::Illuminance must be at c0.w");
static_assert(offsetof(DirectionalLightConstantBufferData, Color) == 16, "DirectionalLightConstantBufferData::Color must start at c1.xyz");
static_assert(
    offsetof(DirectionalLightConstantBufferData, AngularSizeRadians) == 28,
    "DirectionalLightConstantBufferData::AngularSizeRadians must be at c1.w");
static_assert(
    offsetof(DirectionalLightConstantBufferData, CastShadow) == 32,
    "DirectionalLightConstantBufferData::CastShadow must start at c2.x");
static_assert(sizeof(PointLightConstantBufferData) == 64, "Point light constant buffer data must be 64 bytes");
static_assert(offsetof(PointLightConstantBufferData, Position) == 0, "PointLightConstantBufferData::Position must start at c0.xyz");
static_assert(offsetof(PointLightConstantBufferData, Range) == 12, "PointLightConstantBufferData::Range must be at c0.w");
static_assert(offsetof(PointLightConstantBufferData, Color) == 16, "PointLightConstantBufferData::Color must start at c1.xyz");
static_assert(
    offsetof(PointLightConstantBufferData, LuminousIntensity) == 28,
    "PointLightConstantBufferData::LuminousIntensity must be at c1.w");
static_assert(
    offsetof(PointLightConstantBufferData, DistanceAttenuationCoefficients) == 32,
    "PointLightConstantBufferData::DistanceAttenuationCoefficients must start at c2.xyz");
static_assert(offsetof(PointLightConstantBufferData, Radius) == 44, "PointLightConstantBufferData::Radius must be at c2.w");
static_assert(offsetof(PointLightConstantBufferData, CastShadow) == 48, "PointLightConstantBufferData::CastShadow must start at c3.x");
static_assert(sizeof(SpotLightConstantBufferData) == 80, "Spot light constant buffer data must be 80 bytes");
static_assert(offsetof(SpotLightConstantBufferData, Position) == 0, "SpotLightConstantBufferData::Position must start at c0.xyz");
static_assert(offsetof(SpotLightConstantBufferData, Range) == 12, "SpotLightConstantBufferData::Range must be at c0.w");
static_assert(offsetof(SpotLightConstantBufferData, Direction) == 16, "SpotLightConstantBufferData::Direction must start at c1.xyz");
static_assert(offsetof(SpotLightConstantBufferData, InnerAngleCosine) == 28, "SpotLightConstantBufferData::InnerAngleCosine must be at c1.w");
static_assert(offsetof(SpotLightConstantBufferData, Color) == 32, "SpotLightConstantBufferData::Color must start at c2.xyz");
static_assert(
    offsetof(SpotLightConstantBufferData, LuminousIntensity) == 44,
    "SpotLightConstantBufferData::LuminousIntensity must be at c2.w");
static_assert(
    offsetof(SpotLightConstantBufferData, DistanceAttenuationCoefficients) == 48,
    "SpotLightConstantBufferData::DistanceAttenuationCoefficients must start at c3.xyz");
static_assert(offsetof(SpotLightConstantBufferData, Radius) == 60, "SpotLightConstantBufferData::Radius must be at c3.w");
static_assert(
    offsetof(SpotLightConstantBufferData, OuterAngleCosine) == 64,
    "SpotLightConstantBufferData::OuterAngleCosine must start at c4.x");
static_assert(offsetof(SpotLightConstantBufferData, CastShadow) == 68, "SpotLightConstantBufferData::CastShadow must be at c4.y");
static_assert(sizeof(RectLightConstantBufferData) == 64, "Rect light constant buffer data must be 64 bytes");
static_assert(offsetof(RectLightConstantBufferData, Position) == 0, "RectLightConstantBufferData::Position must start at c0.xyz");
static_assert(offsetof(RectLightConstantBufferData, Width) == 12, "RectLightConstantBufferData::Width must be at c0.w");
static_assert(offsetof(RectLightConstantBufferData, Direction) == 16, "RectLightConstantBufferData::Direction must start at c1.xyz");
static_assert(offsetof(RectLightConstantBufferData, Height) == 28, "RectLightConstantBufferData::Height must be at c1.w");
static_assert(offsetof(RectLightConstantBufferData, Tangent) == 32, "RectLightConstantBufferData::Tangent must start at c2.xyz");
static_assert(offsetof(RectLightConstantBufferData, Luminance) == 44, "RectLightConstantBufferData::Luminance must be at c2.w");
static_assert(offsetof(RectLightConstantBufferData, Color) == 48, "RectLightConstantBufferData::Color must start at c3.xyz");
static_assert(offsetof(RectLightConstantBufferData, CastShadow) == 60, "RectLightConstantBufferData::CastShadow must be at c3.w");
static_assert(sizeof(ViewLightingData) == 16, "View lighting constants must occupy one shader constant register");
static_assert(offsetof(ViewLightingData, DirectionalLightCount) == 0, "ViewLightingData::DirectionalLightCount must start at c0.x");
static_assert(offsetof(ViewLightingData, PointLightCount) == 4, "ViewLightingData::PointLightCount must start at c0.y");
static_assert(offsetof(ViewLightingData, SpotLightCount) == 8, "ViewLightingData::SpotLightCount must start at c0.z");
static_assert(offsetof(ViewLightingData, RectLightCount) == 12, "ViewLightingData::RectLightCount must start at c0.w");
