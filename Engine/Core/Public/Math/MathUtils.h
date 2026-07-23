#pragma once

#include "Core/Public/CoreAPI.h"

#include <DirectXMath.h>

#include <cstdint>

namespace MathUtils
{
	SPARKLE_CORE_API std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor) noexcept;
	SPARKLE_CORE_API std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept;
	SPARKLE_CORE_API DirectX::XMFLOAT3 Normalize3(
	    const DirectX::XMFLOAT3& value,
	    const DirectX::XMFLOAT3& fallback = {0.0f, 1.0f, 0.0f},
	    float epsilon = 1e-8f);
	SPARKLE_CORE_API float RadiansToDegrees(float radians);
	SPARKLE_CORE_API DirectX::XMFLOAT3 RadiansToDegrees(const DirectX::XMFLOAT3& radians);
	SPARKLE_CORE_API float DegreesToRadians(float degrees);
	SPARKLE_CORE_API DirectX::XMFLOAT3 DegreesToRadians(const DirectX::XMFLOAT3& degrees);
	SPARKLE_CORE_API DirectX::XMFLOAT4X4 IdentityFloat4x4() noexcept;
	SPARKLE_CORE_API DirectX::XMFLOAT3 DirectionToRotationDegrees(const DirectX::XMFLOAT3& direction) noexcept;
	SPARKLE_CORE_API DirectX::XMFLOAT3 RotationDegreesToDirection(const DirectX::XMFLOAT3& rotationDegrees) noexcept;
	SPARKLE_CORE_API DirectX::XMFLOAT3 ExtractEulerRadians(const DirectX::XMFLOAT4X4& rotationMatrix) noexcept;
}  // namespace MathUtils
