#include "PCH.h"
#include "Scene/Lighting/GameDirectionalLight.h"

#include "Core/Public/Math/MathUtils.h"

#include <algorithm>

DirectionalLightDesc GameDirectionalLight::SanitizeDesc(const DirectionalLightDesc& desc) noexcept
{
	DirectionalLightDesc sanitized = desc;
	sanitized.direction = MathUtils::Normalize3(sanitized.direction, {0.0f, -1.0f, 0.0f});
	sanitized.intensity = (std::max) (0.0f, sanitized.intensity);
	sanitized.color.x = std::clamp(sanitized.color.x, 0.0f, 1.0f);
	sanitized.color.y = std::clamp(sanitized.color.y, 0.0f, 1.0f);
	sanitized.color.z = std::clamp(sanitized.color.z, 0.0f, 1.0f);
	return sanitized;
}