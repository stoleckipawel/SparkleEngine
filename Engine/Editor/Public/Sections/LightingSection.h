#pragma once

#include "Framework/UIRendererSection.h"

#include <cstddef>
#include <string>
#include <vector>

#include <DirectXMath.h>

class LevelManager;
class SceneLighting;

class LightingSection final : public UIRendererSection
{
  public:
	LightingSection(LevelManager& levelManager, SceneLighting& sceneLighting) noexcept;
	~LightingSection() = default;

	LightingSection(const LightingSection&) = delete;
	LightingSection(LightingSection&&) = delete;
	LightingSection& operator=(const LightingSection&) = delete;
	LightingSection& operator=(LightingSection&&) = delete;

	UIRendererSectionId GetId() const noexcept override { return UIRendererSectionId::Lighting; }
	const char* GetTitle() const noexcept override { return "Lighting"; }

	void BuildUI() override;

  private:
	struct LightSelectionEntry
	{
		std::size_t lightIndex = 0;
		std::string label;
	};

	std::vector<LightSelectionEntry> BuildSelectionEntries() const;
	static void ClampLightingUiValues(DirectX::XMFLOAT3& color, float& intensity) noexcept;

	static constexpr float kDirectionSliderMin = -1.0f;
	static constexpr float kDirectionSliderMax = 1.0f;
	static constexpr float kIntensitySliderMin = 0.0f;
	static constexpr float kIntensitySliderMax = 20.0f;

	LevelManager* m_levelManager = nullptr;
	SceneLighting* m_sceneLighting = nullptr;
	int m_selectedLightIndex = 0;
};
