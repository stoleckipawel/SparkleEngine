#include "PCH.h"

#include "Settings/EditorSettingsBootstrap.h"

#include "Settings/EditorRenderingSettings.h"

void ApplyPersistedEditorSettingsToCVars() noexcept
{
	EditorRenderingSettingsSection renderingSettings;
	renderingSettings.ApplyPersistedValuesToRuntimeState();
}
