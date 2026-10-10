#include "PCH.h"

#include "Panels/RenderingExposureSettingsSection.h"

#include "Panels/ExposureSettingsEditor.h"
#include "Panels/RenderingSettingsUi.h"

#include <imgui.h>

void DrawExposureSettingsSection(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings, const char* filterText)
{
	if (!RenderingSettingsUi::MatchesFilter(filterText, "Exposure", "exposure automatic manual metering reduction downsample pyramid compensation ev luminance min max adapt speed")
	    || !RenderingSettingsUi::BeginSettingsCategory("Exposure"))
	{
		return;
	}

	if (RenderingSettingsUi::BeginSettingsTable("##RenderingExposureSettings"))
	{
		ExposureSettingsEditor::DrawSettings(settingsController, settings);
		ImGui::EndTable();
	}
}
