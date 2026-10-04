#pragma once

class EngineRenderingSettingsController;
struct EngineRenderingSettingsState;
struct ViewportExposureOverrides;

class ExposureSettingsEditor final
{
public:
	static void DrawSettings(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings);
	static bool DrawOverrides(ViewportExposureOverrides& overrides, const EngineRenderingSettingsState& defaults) noexcept;
};
