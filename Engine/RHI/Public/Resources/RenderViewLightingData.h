#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>

struct DirectionalLightConstantBufferData
{
	DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
	float Intensity = 1.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};
	float AngularDiameter = 0.009308f;

	std::uint32_t CastShadow = 1u;
	DirectX::XMUINT3 Padding = {0u, 0u, 0u};
};

struct PointLightConstantBufferData
{
	DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
	float Range = 0.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};
	float Intensity = 1.0f;

	float SourceRadius = 0.05f;
	std::uint32_t CastShadow = 1u;
	DirectX::XMUINT2 Padding = {0u, 0u};
};

struct SpotLightConstantBufferData
{
	DirectX::XMFLOAT3 Position = {0.0f, 0.0f, 0.0f};
	float Range = 0.0f;

	DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
	float InnerConeCosine = 1.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};
	float Intensity = 1.0f;

	float OuterConeCosine = 0.0f;
	std::uint32_t CastShadow = 1u;
	float SourceRadius = 0.05f;
	std::uint32_t Padding = 0u;
};

struct ViewLightingData
{
	std::uint32_t DirectionalLightCount = 0;
	std::uint32_t PointLightCount = 0;
	std::uint32_t SpotLightCount = 0;
	std::uint32_t PaddingCount = 0;
};

static_assert(sizeof(DirectionalLightConstantBufferData) == 48, "Directional light constant buffer data must be 48 bytes");
static_assert(
    offsetof(DirectionalLightConstantBufferData, Direction) == 0,
    "DirectionalLightConstantBufferData::Direction must start at c0.xyz");
static_assert(
    offsetof(DirectionalLightConstantBufferData, Intensity) == 12,
    "DirectionalLightConstantBufferData::Intensity must be at c0.w");
static_assert(offsetof(DirectionalLightConstantBufferData, Color) == 16, "DirectionalLightConstantBufferData::Color must start at c1.xyz");
static_assert(
    offsetof(DirectionalLightConstantBufferData, AngularDiameter) == 28,
    "DirectionalLightConstantBufferData::AngularDiameter must be at c1.w");
static_assert(offsetof(DirectionalLightConstantBufferData, CastShadow) == 32, "DirectionalLightConstantBufferData::CastShadow must start at c2.x");
static_assert(sizeof(PointLightConstantBufferData) == 48, "Point light constant buffer data must be 48 bytes");
static_assert(offsetof(PointLightConstantBufferData, Position) == 0, "PointLightConstantBufferData::Position must start at c0.xyz");
static_assert(offsetof(PointLightConstantBufferData, Range) == 12, "PointLightConstantBufferData::Range must be at c0.w");
static_assert(offsetof(PointLightConstantBufferData, Color) == 16, "PointLightConstantBufferData::Color must start at c1.xyz");
static_assert(offsetof(PointLightConstantBufferData, Intensity) == 28, "PointLightConstantBufferData::Intensity must be at c1.w");
static_assert(offsetof(PointLightConstantBufferData, SourceRadius) == 32, "PointLightConstantBufferData::SourceRadius must start at c2.x");
static_assert(offsetof(PointLightConstantBufferData, CastShadow) == 36, "PointLightConstantBufferData::CastShadow must start at c2.y");
static_assert(sizeof(SpotLightConstantBufferData) == 64, "Spot light constant buffer data must be 64 bytes");
static_assert(offsetof(SpotLightConstantBufferData, Position) == 0, "SpotLightConstantBufferData::Position must start at c0.xyz");
static_assert(offsetof(SpotLightConstantBufferData, Range) == 12, "SpotLightConstantBufferData::Range must be at c0.w");
static_assert(offsetof(SpotLightConstantBufferData, Direction) == 16, "SpotLightConstantBufferData::Direction must start at c1.xyz");
static_assert(
    offsetof(SpotLightConstantBufferData, InnerConeCosine) == 28,
    "SpotLightConstantBufferData::InnerConeCosine must be at c1.w");
static_assert(offsetof(SpotLightConstantBufferData, Color) == 32, "SpotLightConstantBufferData::Color must start at c2.xyz");
static_assert(offsetof(SpotLightConstantBufferData, Intensity) == 44, "SpotLightConstantBufferData::Intensity must be at c2.w");
static_assert(
    offsetof(SpotLightConstantBufferData, OuterConeCosine) == 48,
    "SpotLightConstantBufferData::OuterConeCosine must start at c3.x");
static_assert(offsetof(SpotLightConstantBufferData, CastShadow) == 52, "SpotLightConstantBufferData::CastShadow must be at c3.y");
static_assert(offsetof(SpotLightConstantBufferData, SourceRadius) == 56, "SpotLightConstantBufferData::SourceRadius must be at c3.z");
static_assert(sizeof(ViewLightingData) == 16, "View lighting constants must occupy one shader constant register");
static_assert(offsetof(ViewLightingData, DirectionalLightCount) == 0, "ViewLightingData::DirectionalLightCount must start at c0.x");
static_assert(offsetof(ViewLightingData, PointLightCount) == 4, "ViewLightingData::PointLightCount must start at c0.y");
static_assert(offsetof(ViewLightingData, SpotLightCount) == 8, "ViewLightingData::SpotLightCount must start at c0.z");
