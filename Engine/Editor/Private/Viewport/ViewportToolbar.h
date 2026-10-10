#pragma once

#include "Core/Public/Console/CVarControl.h"

#include <memory>
#include <string>
#include <string_view>

class EditorViewportSession;
struct EngineRenderingSettingsState;

// Owns viewport controls and toolbar layout.
// UI owns the borrowed session, rendering defaults and console executor; all
// three outlive this widget. Level names are borrowed only during Draw.
class ViewportToolbar final
{
public:
	ViewportToolbar(
	    EditorViewportSession& viewportSession,
	    const EngineRenderingSettingsState& renderingDefaults,
	    const CVarControlExecutor& consoleVariables) noexcept;
	~ViewportToolbar() noexcept;

	ViewportToolbar(const ViewportToolbar&) = delete;
	ViewportToolbar(ViewportToolbar&&) = delete;
	ViewportToolbar& operator=(const ViewportToolbar&) = delete;
	ViewportToolbar& operator=(ViewportToolbar&&) = delete;

	void SetGeometry(float leftPixels, float topPixels, float widthPixels) noexcept;
	void Draw(std::string_view levelName, bool disableInteraction = false) noexcept;
	float GetHeightPixels() const noexcept { return m_heightPixels; }

private:
	void DrawLevelName(std::string_view levelName, bool compact) const noexcept;
	void DrawViewModeSelector(bool disableInteraction, bool compact) noexcept;
	void DrawCameraControls(bool disableInteraction, bool compact) noexcept;
	void DrawFrameStats() const noexcept;

	EditorViewportSession& m_viewportSession;
	const EngineRenderingSettingsState& m_renderingDefaults;
	const CVarControlExecutor& m_consoleVariables;
	std::string m_showControlError;
	float m_leftPixels = 0.0f;
	float m_topPixels = 0.0f;
	float m_widthPixels = 0.0f;
	float m_heightPixels = 0.0f;
};
