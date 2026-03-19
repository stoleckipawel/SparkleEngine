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
}  // namespace MathUtils
