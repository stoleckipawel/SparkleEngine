#pragma once

#include "Core/Public/Rendering/RendererSettings.h"

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

struct PointLightConstantBufferData
{
	DirectX::XMFLOAT3 Position;
	float Intensity;

	DirectX::XMFLOAT3 Color;
	float Radius;

	std::uint32_t Enabled;
	DirectX::XMFLOAT3 Padding;
};

struct PerViewLightingConstantBufferData
{
	static constexpr std::size_t MaxDirectionalLights = RendererSettings::Lights::MaxDirectionalLights;
	static constexpr std::size_t MaxPointLights = RendererSettings::Lights::MaxPointLights;

	std::uint32_t DirectionalLightCount = 0;
	std::uint32_t PointLightCount = 0;
	std::uint32_t PaddingCounts[2] = {};

	DirectionalLightConstantBufferData DirectionalLights[MaxDirectionalLights] = {};
	PointLightConstantBufferData PointLights[MaxPointLights] = {};
	std::array<DirectX::XMFLOAT4, 13> Padding = {};
};

static_assert(sizeof(DirectionalLightConstantBufferData) == 32, "Directional light constant buffer data must be 32 bytes");
static_assert(sizeof(PointLightConstantBufferData) == 48, "Point light constant buffer data must be 48 bytes");
static_assert(sizeof(PerViewLightingConstantBufferData) == 12576, "Per-view lighting constant buffer data must match the shader layout");