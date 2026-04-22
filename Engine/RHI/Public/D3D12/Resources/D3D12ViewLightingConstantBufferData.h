#pragma once

#include "D3D12ShadowConstantBufferData.h"
#include "Config/RenderConfig.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>

struct DirectionalLightConstantBufferData
{
	DirectX::XMFLOAT3 Direction = {0.0f, -1.0f, 0.0f};
	float Intensity = 1.0f;

	DirectX::XMFLOAT3 Color = {1.0f, 1.0f, 1.0f};
	std::uint32_t CastShadow = 1u;
};

struct PerViewLightingConstantBufferData
{
	static constexpr std::size_t MaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;
	static constexpr std::size_t MaxShadowCascades = RenderConfig::Shadows::MaxCascades;
	static constexpr std::size_t MaxShadowMaps = RenderConfig::Shadows::MaxShadowMaps;

	std::uint32_t DirectionalLightCount = 0;
	std::uint32_t PaddingCounts[3] = {};

	DirectionalLightConstantBufferData DirectionalLights[MaxDirectionalLights] = {};
	ShadowConstantBufferData Shadows[MaxShadowMaps] = {};
};

static_assert(sizeof(DirectionalLightConstantBufferData) == 32, "Directional light constant buffer data must be 32 bytes");
static_assert(
    offsetof(DirectionalLightConstantBufferData, Direction) == 0,
    "DirectionalLightConstantBufferData::Direction must start at c0.xyz");
static_assert(
    offsetof(DirectionalLightConstantBufferData, Intensity) == 12,
    "DirectionalLightConstantBufferData::Intensity must be at c0.w");
static_assert(offsetof(DirectionalLightConstantBufferData, Color) == 16, "DirectionalLightConstantBufferData::Color must start at c1.xyz");
static_assert(offsetof(DirectionalLightConstantBufferData, CastShadow) == 28, "DirectionalLightConstantBufferData::CastShadow must be at c1.w");
static_assert(sizeof(PerViewLightingConstantBufferData) == 400, "Per-view lighting constant buffer data must match the shader layout");
static_assert(
    offsetof(PerViewLightingConstantBufferData, DirectionalLightCount) == 0,
    "PerViewLightingConstantBufferData::DirectionalLightCount must start at c0.x");
static_assert(
    offsetof(PerViewLightingConstantBufferData, DirectionalLights) == 16,
    "PerViewLightingConstantBufferData::DirectionalLights must start at c1");
static_assert(
    offsetof(PerViewLightingConstantBufferData, Shadows) == 80,
    "PerViewLightingConstantBufferData::Shadows must start after the directional light array");