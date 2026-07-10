#pragma once

#include "Core/Public/Hash/HashUtils.h"

#include <DirectXMath.h>

#include <cstdint>

namespace LightingStateHash
{
	inline std::uint64_t AppendBool(std::uint64_t hash, bool value) noexcept
	{
		return Hash::ContinueFnv1a64Value(hash, static_cast<std::uint8_t>(value));
	}

	inline std::uint64_t AppendFloat3(std::uint64_t hash, const DirectX::XMFLOAT3& value) noexcept
	{
		hash = Hash::ContinueFnv1a64Value(hash, value.x);
		hash = Hash::ContinueFnv1a64Value(hash, value.y);
		return Hash::ContinueFnv1a64Value(hash, value.z);
	}

	inline std::uint64_t AppendFloat4(std::uint64_t hash, const DirectX::XMFLOAT4& value) noexcept
	{
		hash = AppendFloat3(hash, DirectX::XMFLOAT3{value.x, value.y, value.z});
		return Hash::ContinueFnv1a64Value(hash, value.w);
	}

	inline std::uint64_t AppendMatrix(std::uint64_t hash, const DirectX::XMFLOAT4X4& value) noexcept
	{
		for (std::uint32_t row = 0u; row < 4u; ++row)
		{
			for (std::uint32_t column = 0u; column < 4u; ++column)
			{
				hash = Hash::ContinueFnv1a64Value(hash, value.m[row][column]);
			}
		}
		return hash;
	}
}
