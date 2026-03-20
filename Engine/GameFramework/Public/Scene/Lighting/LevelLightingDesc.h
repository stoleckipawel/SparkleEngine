#pragma once

#include "RenderConfig.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightDesc.h"
#include "GameFramework/Public/Scene/Lighting/PointLightDesc.h"

#include <array>
#include <cstddef>
#include <cstdint>

struct SPARKLE_ENGINE_API LevelLightingDesc
{
	static constexpr std::size_t MaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;
	static constexpr std::size_t MaxPointLights = RenderConfig::Lights::MaxPointLights;

	std::uint32_t directionalLightCount = 0;
	std::uint32_t pointLightCount = 0;

	std::array<DirectionalLightDesc, MaxDirectionalLights> directionalLights = {};
	std::array<PointLightDesc, MaxPointLights> pointLights = {};
};