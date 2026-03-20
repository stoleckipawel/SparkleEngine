#pragma once

#include "RenderConfig.h"

#include <DirectXMath.h>

#include <array>
#include <cstddef>
#include <cstdint>

struct DirectionalLightConstantBufferData
{
	DirectX::XMFLOAT3 Direction;
	float Intensity;

	DirectX::XMFLOAT3 Color;
	float Padding;
};

struct PerViewLightingConstantBufferData
{
	static constexpr std::size_t MaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;

	std::uint32_t DirectionalLightCount = 0;
	std::uint32_t PaddingCounts[3] = {};

	DirectionalLightConstantBufferData DirectionalLights[MaxDirectionalLights] = {};
};

static_assert(sizeof(DirectionalLightConstantBufferData) == 32, "Directional light constant buffer data must be 32 bytes");
static_assert(sizeof(PerViewLightingConstantBufferData) == 80, "Per-view lighting constant buffer data must match the shader layout");