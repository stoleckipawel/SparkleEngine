#pragma once

#include "RenderConfig.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/LevelLightingDesc.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

class SPARKLE_ENGINE_API GameSceneLightingState final
{
  public:
	static constexpr std::size_t MaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;

	GameSceneLightingState() noexcept = default;
	~GameSceneLightingState() noexcept = default;

	GameSceneLightingState(const GameSceneLightingState&) = delete;
	GameSceneLightingState& operator=(const GameSceneLightingState&) = delete;
	GameSceneLightingState(GameSceneLightingState&&) = delete;
	GameSceneLightingState& operator=(GameSceneLightingState&&) = delete;

	const DirectionalLightDesc& GetDirectionalLight(std::size_t index) const noexcept { return m_lightingDesc.directionalLights[index]; }
	const LevelLightingDesc& GetLightingDesc() const noexcept { return m_lightingDesc; }
	std::uint32_t GetDirectionalLightCount() const noexcept { return m_lightingDesc.directionalLightCount; }

	void SetDirectionalLightCount(std::uint32_t count) noexcept
	{
		m_lightingDesc.directionalLightCount = std::min<std::uint32_t>(count, static_cast<std::uint32_t>(MaxDirectionalLights));
	}

	void SetDirectionalLight(std::size_t index, const DirectionalLightDesc& directionalLight) noexcept
	{
		if (index >= MaxDirectionalLights)
		{
			return;
		}

		m_lightingDesc.directionalLightCount = std::max(m_lightingDesc.directionalLightCount, static_cast<std::uint32_t>(index + 1));
		m_lightingDesc.directionalLights[index] = DirectionalLightDesc::Sanitize(directionalLight);
	}

	void ApplyLevelLightingDesc(const LevelLightingDesc& lightingDesc) noexcept
	{
		m_lightingDesc = {};
		SetDirectionalLightCount(lightingDesc.directionalLightCount);

		for (std::size_t lightIndex = 0; lightIndex < m_lightingDesc.directionalLightCount; ++lightIndex)
		{
			SetDirectionalLight(lightIndex, lightingDesc.directionalLights[lightIndex]);
		}
	}

	LevelLightingDesc CaptureLevelLightingDesc() const noexcept { return m_lightingDesc; }

	void Reset() noexcept { m_lightingDesc = {}; }

	private:
	LevelLightingDesc m_lightingDesc = {};
};