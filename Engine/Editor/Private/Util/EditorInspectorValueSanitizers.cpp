#include "PCH.h"
#include "Util/EditorInspectorValueSanitizers.h"

#include <algorithm>

namespace EditorInspectorValueSanitizers
{
	void ClampCameraValues(float& fovYDegrees, float& moveSpeed) noexcept
	{
		fovYDegrees = std::clamp(fovYDegrees, 1.0f, 179.0f);
		moveSpeed = std::clamp(moveSpeed, 0.0001f, 10.0f);
	}

	void ClampLightValues(DirectX::XMFLOAT3& color, float& intensity) noexcept
	{
		color.x = std::clamp(color.x, 0.0f, 1.0f);
		color.y = std::clamp(color.y, 0.0f, 1.0f);
		color.z = std::clamp(color.z, 0.0f, 1.0f);
		intensity = (std::max) (0.0f, intensity);
	}
}  // namespace EditorInspectorValueSanitizers
