#pragma once

#include "Framework/UIRendererSection.h"

class LevelManager;

class SceneSection final : public UIRendererSection
{
  public:
	explicit SceneSection(LevelManager& levelManager) noexcept;
	~SceneSection() = default;

	SceneSection(const SceneSection&) = delete;
	SceneSection(SceneSection&&) = delete;
	SceneSection& operator=(const SceneSection&) = delete;
	SceneSection& operator=(SceneSection&&) = delete;

	UIRendererSectionId GetId() const noexcept override { return UIRendererSectionId::Scene; }
	const char* GetTitle() const noexcept override { return "Scene"; }

	void BuildUI() override;

  private:
	LevelManager* m_levelManager = nullptr;
};