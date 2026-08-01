#pragma once

#include <DirectXMath.h>

class GltfVertexFrameBuilder final
{
  public:
	static DirectX::XMFLOAT3 BuildNormal(const DirectX::XMFLOAT3& normal);
	static DirectX::XMFLOAT4 BuildAuthoredTangent(const DirectX::XMFLOAT4& tangent, const DirectX::XMFLOAT3& normalizedNormal);

  private:
	static constexpr float kMinimumDirectionLengthSquared = 1.0e-12f;
	static constexpr float kUnitFrameTolerance = 1.0e-3f;

	static bool IsFinite(const DirectX::XMFLOAT3& value) noexcept;
};
