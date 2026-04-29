#pragma once

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace MathUtils
{
	std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor) noexcept;
	std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept;

	inline DirectX::XMFLOAT3 Normalize3(
	    const DirectX::XMFLOAT3& v,
	    const DirectX::XMFLOAT3& fallback = {0.0f, 1.0f, 0.0f},
	    float epsilon = 1e-8f)
	{
		float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
		if (len2 < epsilon)
			return fallback;
		float invLen = 1.0f / std::sqrt(len2);
		return {v.x * invLen, v.y * invLen, v.z * invLen};
	}

	inline float RadiansToDegrees(float radians)
	{
		return DirectX::XMConvertToDegrees(radians);
	}

	inline DirectX::XMFLOAT3 RadiansToDegrees(const DirectX::XMFLOAT3& radians)
	{
		return {RadiansToDegrees(radians.x), RadiansToDegrees(radians.y), RadiansToDegrees(radians.z)};
	}

	inline float DegreesToRadians(float degrees)
	{
		return DirectX::XMConvertToRadians(degrees);
	}

	inline DirectX::XMFLOAT3 DegreesToRadians(const DirectX::XMFLOAT3& degrees)
	{
		return {DegreesToRadians(degrees.x), DegreesToRadians(degrees.y), DegreesToRadians(degrees.z)};
	}

	inline DirectX::XMFLOAT3 DirectionToRotationDegrees(const DirectX::XMFLOAT3& direction) noexcept
	{
		const float clampedY = (std::max) (-1.0f, (std::min) (1.0f, direction.y));
		const float pitch = std::asin(clampedY);
		const float yaw = std::atan2(direction.x, direction.z);
		return {RadiansToDegrees(pitch), RadiansToDegrees(yaw), 0.0f};
	}

	inline DirectX::XMFLOAT3 RotationDegreesToDirection(const DirectX::XMFLOAT3& rotationDegrees) noexcept
	{
		const float pitch = DegreesToRadians(rotationDegrees.x);
		const float yaw = DegreesToRadians(rotationDegrees.y);
		const float cosPitch = std::cos(pitch);
		return {std::sin(yaw) * cosPitch, std::sin(pitch), std::cos(yaw) * cosPitch};
	}

	inline DirectX::XMFLOAT3 ExtractEulerRadians(const DirectX::XMFLOAT4X4& rotationMatrix) noexcept
	{
		const float sinPitch = (std::max) (-1.0f, (std::min) (1.0f, -rotationMatrix._23));
		const float pitch = std::asin(sinPitch);

		float yaw = 0.0f;
		float roll = 0.0f;
		if (std::abs(std::cos(pitch)) > 0.0001f)
		{
			yaw = std::atan2(rotationMatrix._13, rotationMatrix._33);
			roll = std::atan2(rotationMatrix._21, rotationMatrix._22);
		}
		else
		{
			yaw = std::atan2(-rotationMatrix._31, rotationMatrix._11);
		}

		return {pitch, yaw, roll};
	}
}  // namespace MathUtils
