#pragma once

#include "Core/Public/Hash/HashUtils.h"

#include <DirectXMath.h>

#include <cstdint>

namespace LightingStateHash
{
	std::uint64_t AppendBool(std::uint64_t hash, bool value) noexcept;
	std::uint64_t AppendFloat3(std::uint64_t hash, const DirectX::XMFLOAT3& value) noexcept;
	std::uint64_t AppendFloat4(std::uint64_t hash, const DirectX::XMFLOAT4& value) noexcept;
	std::uint64_t AppendMatrix(std::uint64_t hash, const DirectX::XMFLOAT4X4& value) noexcept;
}
