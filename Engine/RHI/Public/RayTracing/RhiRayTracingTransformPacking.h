#pragma once

#include <DirectXMath.h>

#include <array>

namespace RhiRayTracingTransformPacking
{
	// D3D12 and Vulkan acceleration-structure instances use a row-major 3x4
	// column-vector affine transform. Sparkle stores canonical transforms for
	// row-vector multiplication, so the native payload is the affine transpose.
	inline std::array<float, 12> PackCanonicalObjectToWorld(const DirectX::XMFLOAT4X4& worldMatrix) noexcept
	{
		return {
		    worldMatrix._11,
		    worldMatrix._21,
		    worldMatrix._31,
		    worldMatrix._41,
		    worldMatrix._12,
		    worldMatrix._22,
		    worldMatrix._32,
		    worldMatrix._42,
		    worldMatrix._13,
		    worldMatrix._23,
		    worldMatrix._33,
		    worldMatrix._43};
	}
}
