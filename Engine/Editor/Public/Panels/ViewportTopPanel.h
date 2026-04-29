#pragma once

#include "../../../Renderer/Public/Debug/RenderViewMode.h"

class LevelManager;

class ViewportTopPanel final
{
  public:
	ViewportTopPanel(LevelManager* levelManager = nullptr) noexcept;
	~ViewportTopPanel() = default;

	ViewportTopPanel(const ViewportTopPanel&) = delete;
	ViewportTopPanel(ViewportTopPanel&&) = delete;
	ViewportTopPanel& operator=(const ViewportTopPanel&) = delete;
	ViewportTopPanel& operator=(ViewportTopPanel&&) = delete;

	void SetLevelManager(LevelManager* levelManager) noexcept;
	void SetGeometry(float leftPixels, float topPixels, float widthPixels) noexcept;
	void BuildUI(bool disableInteraction = false) noexcept;
	float GetHeight() const noexcept { return m_heightPixels; }

  private:
	static const char* GetViewModeLabel(RenderViewMode viewMode) noexcept;
	static void DrawViewModeCategory(const char* label) noexcept;
	static void DrawViewModeOption(RenderViewMode option, RenderViewMode currentViewMode) noexcept;

	void BuildLevelName() const noexcept;
	void BuildViewModeCombo(bool disableInteraction) noexcept;
	void BuildPerformanceStats() const noexcept;

	LevelManager* m_levelManager = nullptr;
	float m_leftPixels = 0.0f;
	float m_topPixels = 0.0f;
	float m_widthPixels = 0.0f;
	float m_heightPixels = 0.0f;
};