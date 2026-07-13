#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

struct SkyUniformData
{
	DirectX::XMFLOAT3 Color{1.0f, 1.0f, 1.0f};
	float Intensity = 1.0f;
	std::uint32_t Enabled = 1u;
	DirectX::XMUINT3 Padding{};
};

static_assert(std::is_standard_layout_v<SkyUniformData>);
static_assert(std::is_trivially_copyable_v<SkyUniformData>);
static_assert(sizeof(SkyUniformData) == 32);
