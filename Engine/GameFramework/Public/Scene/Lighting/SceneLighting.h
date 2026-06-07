#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Lighting/LevelLightingDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"

#include <cstddef>
#include <vector>

class SPARKLE_ENGINE_API SceneLighting final
{
  public:
	SceneLighting() noexcept = default;
	~SceneLighting() noexcept = default;

	SceneLighting(const SceneLighting&) = delete;
	SceneLighting& operator=(const SceneLighting&) = delete;
	SceneLighting(SceneLighting&&) = delete;
	SceneLighting& operator=(SceneLighting&&) = delete;

	std::size_t GetLightCount() const noexcept { return m_lights.size(); }

	const std::vector<SceneLightDesc>& GetLights() const noexcept { return m_lights; }
	const SceneLightDesc* GetLight(std::size_t index) const noexcept;
	bool IsLightVisible(std::size_t index) const noexcept;
	void SetLightVisible(std::size_t index, bool visible);
	bool ApplyLightDesc(std::size_t lightIndex, SceneLightDesc light);

	void ApplyFromDesc(const LevelLightingDesc& desc);
	void AppendLight(SceneLightDesc light);
	LevelLightingDesc CaptureToDesc() const noexcept;
	LightingSnapshot CaptureSnapshot() const noexcept;

	void Reset() noexcept;

  private:
	std::vector<SceneLightDesc> m_lights;
};
