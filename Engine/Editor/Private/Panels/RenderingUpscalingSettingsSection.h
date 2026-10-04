#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsController;

void DrawUpscalingSettingsSection(
    EngineRenderingSettingsController& settingsController,
    const EngineRenderingSettingsState& settings,
    const char* filterText);
