#pragma once

#include "RenderConfig.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightDesc.h"

#include <array>
#include <cstddef>
#include <cstdint>

struct SPARKLE_ENGINE_API LightingSnapshot
{
	static constexpr std::size_t MaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;

	std::uint32_t directionalLightCount = 0;
	std::array<DirectionalLightDesc, MaxDirectionalLights> directionalLights = {};
};
