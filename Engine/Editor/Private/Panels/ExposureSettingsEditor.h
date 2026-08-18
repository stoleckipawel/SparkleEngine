#pragma once

class EngineRenderingSettingsSection;
struct EngineRenderingSettingsState;
struct ViewportExposureOverrides;

class ExposureSettingsEditor final
{
public:
	static void DrawSettings(EngineRenderingSettingsSection& settingsSection, const EngineRenderingSettingsState& settings);
	static bool DrawOverrides(ViewportExposureOverrides& overrides, const EngineRenderingSettingsState& defaults) noexcept;
};
