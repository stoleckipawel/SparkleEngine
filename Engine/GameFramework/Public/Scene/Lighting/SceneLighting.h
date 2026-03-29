#pragma once

#include "RenderConfig.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightComponent.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Lighting/LevelLightingDesc.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class SPARKLE_ENGINE_API SceneLighting final
{
  public:
	static constexpr std::size_t MaxDirectionalLights = RenderConfig::Lights::MaxDirectionalLights;

	SceneLighting() noexcept = default;
	~SceneLighting() noexcept = default;

	SceneLighting(const SceneLighting&) = delete;
	SceneLighting& operator=(const SceneLighting&) = delete;
	SceneLighting(SceneLighting&&) = delete;
	SceneLighting& operator=(SceneLighting&&) = delete;

	std::size_t GetDirectionalLightCount() const noexcept { return m_directionalLightComponents.size(); }

	const DirectionalLightComponent& GetDirectionalLightComponent(std::size_t index) const noexcept { return m_directionalLightComponents[index]; }
	DirectionalLightComponent& GetDirectionalLightComponent(std::size_t index) noexcept { return m_directionalLightComponents[index]; }

	const std::vector<DirectionalLightComponent>& GetDirectionalLightComponents() const noexcept { return m_directionalLightComponents; }

	void ApplyFromDesc(const LevelLightingDesc& desc) noexcept;
	LevelLightingDesc CaptureToDesc() const noexcept;
	LightingSnapshot CaptureSnapshot() const noexcept;

	void Reset() noexcept { m_directionalLightComponents.clear(); }

  private:
	std::vector<DirectionalLightComponent> m_directionalLightComponents;
};
