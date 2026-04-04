#pragma once

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace MathUtils
{
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

	inline float DegreesToRadians(float degrees)
	{
		return DirectX::XMConvertToRadians(degrees);
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
