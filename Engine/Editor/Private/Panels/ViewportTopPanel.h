#pragma once

#include "Core/Public/Console/CVarControl.h"

#include <string>

class LevelSession;
class EngineRenderingSettingsController;
class EditorViewportSession;

class ViewportTopPanel final
{
public:
	ViewportTopPanel(
	    LevelSession* levelSession = nullptr,
	    EngineRenderingSettingsController* renderingSettings = nullptr,
	    EditorViewportSession* viewportSession = nullptr,
	    const CVarControlExecutor* consoleVariables = nullptr) noexcept;
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
	void BuildLevelName(bool compact) const noexcept;
	void BuildViewModeCombo(bool disableInteraction, bool compact) noexcept;
	void BuildCameraControls(bool disableInteraction, bool compact) noexcept;
	void BuildFrameStats() const noexcept;

	LevelSession* m_levelSession = nullptr;
	EngineRenderingSettingsController* m_renderingSettings = nullptr;
	EditorViewportSession* m_viewportSession = nullptr;
	const CVarControlExecutor* m_consoleVariables = nullptr;
	std::string m_showControlError;
	float m_leftPixels = 0.0f;
	float m_topPixels = 0.0f;
	float m_widthPixels = 0.0f;
	float m_heightPixels = 0.0f;
};
