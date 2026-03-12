#pragma once

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace MathUtils
{

	inline DirectX::XMFLOAT3 Add(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		return {a.x + b.x, a.y + b.y, a.z + b.z};
	}

	inline DirectX::XMFLOAT3 Sub(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		return {a.x - b.x, a.y - b.y, a.z - b.z};
	}

	inline DirectX::XMFLOAT3 Mul(const DirectX::XMFLOAT3& a, float s)
	{
		return {a.x * s, a.y * s, a.z * s};
	}

	inline float Dot(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline DirectX::XMFLOAT3 Cross(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}

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

	inline DirectX::XMFLOAT2 SphericalUV(const DirectX::XMFLOAT3& n)
	{
		float u = std::atan2(n.z, n.x) / DirectX::XM_2PI + 0.5f;
		float v = std::acosf(std::clamp(n.y, -1.0f, 1.0f)) / DirectX::XM_PI;
		return {u, v};
	}

	inline uint64_t EdgeKey(uint32_t a, uint32_t b)
	{
		uint32_t lo = std::min(a, b);
		uint32_t hi = std::max(a, b);
		return (uint64_t) lo | ((uint64_t) hi << 32);
	}

}  // namespace MathUtils
