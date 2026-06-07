#pragma once

#include "Config/RenderConfig.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightDesc.h"
#include "GameFramework/Public/Scene/Lighting/PointLightSnapshotDesc.h"
#include "GameFramework/Public/Scene/Lighting/SpotLightSnapshotDesc.h"

#include <array>
#include <cstddef>
#include <cstdint>

struct SPARKLE_ENGINE_API LightingSnapshot
{
	static constexpr std::size_t MaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;
	static constexpr std::size_t MaxPointLights = RenderConfig::Lights::MaxPointLights;
	static constexpr std::size_t MaxSpotLights = RenderConfig::Lights::MaxSpotLights;

	std::uint32_t directionalLightCount = 0;
	std::uint32_t pointLightCount = 0;
	std::uint32_t spotLightCount = 0;
	std::uint32_t padding = 0;
	std::array<DirectionalLightDesc, MaxDirectionalLights> directionalLights = {};
	std::array<PointLightSnapshotDesc, MaxPointLights> pointLights = {};
	std::array<SpotLightSnapshotDesc, MaxSpotLights> spotLights = {};
};
