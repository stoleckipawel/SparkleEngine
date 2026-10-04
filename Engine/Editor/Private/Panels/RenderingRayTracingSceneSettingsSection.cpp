#include "PCH.h"

#include "Panels/RenderingRayTracingSceneSettingsSection.h"

#include "Panels/RenderingSettingsUi.h"
#include "Panels/RenderingPtlasSettingsSection.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsController.h"

#include <imgui.h>

void DrawRayTracingSceneSettingsSection(
    EngineRenderingSettingsController& settingsController,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	if (!RenderingSettingsUi::MatchesFilter(
	        filterText,
	        "Ray Tracing Scene",
	        "ray tracing scene tlas refit ptlas active partition update mode partitions dynamic distance acceleration structure")
	    || !RenderingSettingsUi::BeginSettingsCategory("Ray Tracing Scene"))
	{
		return;
	}

	if (RenderingSettingsUi::BeginSettingsTable("##RenderingRayTracingSceneSettings"))
	{
		ImGui::BeginDisabled(settings.PtlasActive);
		RenderingSettingsUi::DrawBooleanRow(
		    "##RefitTlas",
		    "Refit TLAS",
		    settings.RefitTlas,
		    [&settingsController](bool value) { settingsController.SetRefitTlas(value); });
		ImGui::EndDisabled();
		ImGui::EndTable();
	}

	DrawPtlasSettingsSection(settingsController, settings);
}
