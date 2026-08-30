#include "PCH.h"

#include "Core/Public/Math/MathUtils.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace MathUtils
{
	std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor) noexcept
	{
		assert(divisor > 0);
		return (value + divisor - 1) / divisor;
	}

	std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept
	{
		assert(alignment > 0);
		return (value + alignment - 1) & ~(alignment - 1);
	}

	DirectX::XMFLOAT3 Normalize3(const DirectX::XMFLOAT3& value, const DirectX::XMFLOAT3& fallback, float epsilon)
	{
		const float lengthSquared = value.x * value.x + value.y * value.y + value.z * value.z;
		if (lengthSquared < epsilon)
		{
			return fallback;
		}

		const float inverseLength = 1.0f / std::sqrt(lengthSquared);
		return {value.x * inverseLength, value.y * inverseLength, value.z * inverseLength};
	}

	float RadiansToDegrees(float radians)
	{
		return DirectX::XMConvertToDegrees(radians);
	}

	DirectX::XMFLOAT3 RadiansToDegrees(const DirectX::XMFLOAT3& radians)
	{
		return {RadiansToDegrees(radians.x), RadiansToDegrees(radians.y), RadiansToDegrees(radians.z)};
	}

	float DegreesToRadians(float degrees)
	{
		return DirectX::XMConvertToRadians(degrees);
	}

	DirectX::XMFLOAT3 DegreesToRadians(const DirectX::XMFLOAT3& degrees)
	{
		return {DegreesToRadians(degrees.x), DegreesToRadians(degrees.y), DegreesToRadians(degrees.z)};
	}

	DirectX::XMFLOAT4X4 IdentityFloat4x4() noexcept
	{
		return {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	}

	DirectX::XMFLOAT3 DirectionToRotationDegrees(const DirectX::XMFLOAT3& direction) noexcept
	{
		const float clampedY = (std::max) (-1.0f, (std::min) (1.0f, direction.y));
		const float pitch = std::asin(clampedY);
		const float yaw = std::atan2(direction.x, direction.z);
		return {RadiansToDegrees(pitch), RadiansToDegrees(yaw), 0.0f};
	}

	DirectX::XMFLOAT3 RotationDegreesToDirection(const DirectX::XMFLOAT3& rotationDegrees) noexcept
	{
		const float pitch = DegreesToRadians(rotationDegrees.x);
		const float yaw = DegreesToRadians(rotationDegrees.y);
		const float cosinePitch = std::cos(pitch);
		return {std::sin(yaw) * cosinePitch, std::sin(pitch), std::cos(yaw) * cosinePitch};
	}

	DirectX::XMFLOAT3 ExtractEulerRadians(const DirectX::XMFLOAT4X4& rotationMatrix) noexcept
	{
		const float sinePitch = (std::max) (-1.0f, (std::min) (1.0f, -rotationMatrix._23));
		const float pitch = std::asin(sinePitch);

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
}
