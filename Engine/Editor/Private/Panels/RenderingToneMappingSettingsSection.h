#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsController;

void DrawToneMappingSettingsSection(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings, const char* filterText);
