#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsSection;

void DrawUpscalingSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText);
