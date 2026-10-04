#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsController;

void DrawRayTracingSceneSettingsSection(
    EngineRenderingSettingsController& settingsController,
    const EngineRenderingSettingsState& settings,
    const char* filterText);
