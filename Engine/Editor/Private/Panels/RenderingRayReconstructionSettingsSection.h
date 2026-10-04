#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsController;

void DrawRayReconstructionSettingsSection(
    EngineRenderingSettingsController& settingsController,
    const EngineRenderingSettingsState& settings,
    const char* filterText);
