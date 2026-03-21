#pragma once

#include "Framework/UIRendererSection.h"

#include <cstddef>
#include <string>
#include <vector>

class LevelManager;

class LightingSection final : public UIRendererSection
{
  public:
	explicit LightingSection(LevelManager& levelManager) noexcept;
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

	LevelManager* m_levelManager = nullptr;
	int m_selectedLightIndex = 0;
};
