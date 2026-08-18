#pragma once

class EditorViewportSession;
struct EngineRenderingSettingsState;

class ViewportCameraProperties final
{
public:
	static void OpenPopup() noexcept;
	static void BuildPopup(
	    EditorViewportSession& viewportSession,
	    const EngineRenderingSettingsState& renderingDefaults,
	    bool disableInteraction) noexcept;
};
