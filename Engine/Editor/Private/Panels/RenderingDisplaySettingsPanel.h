#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsSection;

void DrawDisplaySettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText);
