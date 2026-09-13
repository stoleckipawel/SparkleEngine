#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

struct MaterialTextureMappingData final
{
	DirectX::XMFLOAT4 UvLinear = {1.0f, 0.0f, 0.0f, 1.0f};
	DirectX::XMFLOAT2 UvOffset = {0.0f, 0.0f};
	float Strength = 1.0f;
	std::uint32_t AddressModes = 0u;
};
static_assert(std::is_standard_layout_v<MaterialTextureMappingData>);
static_assert(std::is_trivially_copyable_v<MaterialTextureMappingData>);
static_assert(sizeof(MaterialTextureMappingData) == 32);
