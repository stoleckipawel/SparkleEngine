#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsSection;

void DrawRayReconstructionSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText);
