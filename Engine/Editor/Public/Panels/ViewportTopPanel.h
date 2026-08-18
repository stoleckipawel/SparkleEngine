#pragma once

#include "../../../Renderer/Public/Debug/RenderViewMode.h"

class LevelSession;
class EngineRenderingSettingsSection;
class EditorViewportSession;

class ViewportTopPanel final
{
public:
	ViewportTopPanel(
	    LevelSession* levelSession = nullptr,
	    EngineRenderingSettingsSection* renderingSettings = nullptr,
	    EditorViewportSession* viewportSession = nullptr) noexcept;
	~ViewportTopPanel() noexcept;

	ViewportTopPanel(const ViewportTopPanel&) = delete;
	ViewportTopPanel(ViewportTopPanel&&) = delete;
	ViewportTopPanel& operator=(const ViewportTopPanel&) = delete;
	ViewportTopPanel& operator=(ViewportTopPanel&&) = delete;

	void SetLevelSession(LevelSession* levelSession) noexcept;
	void SetGeometry(float leftPixels, float topPixels, float widthPixels) noexcept;
	void BuildUI(bool disableInteraction = false) noexcept;
	float GetHeight() const noexcept { return m_heightPixels; }

private:
	static const char* GetViewModeLabel(RenderViewMode viewMode) noexcept;
	static void DrawViewModeCategory(const char* label) noexcept;
	void DrawViewModeOption(RenderViewMode option, RenderViewMode currentViewMode) noexcept;

	void BuildLevelName(bool compact) const noexcept;
	void BuildViewModeCombo(bool disableInteraction, bool compact) noexcept;
	void BuildRightControls(bool disableInteraction, bool compact) noexcept;

	LevelSession* m_levelSession = nullptr;
	EngineRenderingSettingsSection* m_renderingSettings = nullptr;
	EditorViewportSession* m_viewportSession = nullptr;
	float m_leftPixels = 0.0f;
	float m_topPixels = 0.0f;
	float m_widthPixels = 0.0f;
	float m_heightPixels = 0.0f;
};
