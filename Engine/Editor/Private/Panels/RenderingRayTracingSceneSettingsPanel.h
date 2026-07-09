#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsSection;

void DrawRayTracingSceneSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText);
