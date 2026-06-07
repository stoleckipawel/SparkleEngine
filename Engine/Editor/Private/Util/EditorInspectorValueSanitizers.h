#pragma once

#include <DirectXMath.h>

namespace EditorInspectorValueSanitizers
{
	void ClampCameraValues(float& fovYDegrees, float& moveSpeed) noexcept;
	void ClampLightValues(DirectX::XMFLOAT3& color, float& intensity) noexcept;
}  // namespace EditorInspectorValueSanitizers
