#pragma once

struct EngineRenderingSettingsState;
class EngineRenderingSettingsController;

void DrawDisplaySettingsSection(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings, const char* filterText);
