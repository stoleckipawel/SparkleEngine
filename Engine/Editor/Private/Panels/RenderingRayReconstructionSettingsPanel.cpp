#include "PCH.h"

#include "Panels/RenderingRayReconstructionSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsSection.h"

#include <imgui.h>

void DrawRayReconstructionSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	static constexpr RenderingSettingsPanelUi::ComboOption<EngineRayReconstructionMode> rayReconstructionModeOptions[] = {
	    {"Off", EngineRayReconstructionMode::Off},
	    {"NVIDIA DLSS Ray Reconstruction", EngineRayReconstructionMode::NvidiaDlssRayReconstruction},
	};

	if (!RenderingSettingsPanelUi::MatchesFilter(filterText, "Ray Reconstruction", "dlss ray reconstruction indirect specular diffuse")
	    || !RenderingSettingsPanelUi::BeginSettingsCategory("Ray Reconstruction"))
	{
		return;
	}

	if (RenderingSettingsPanelUi::BeginSettingsTable("##RenderingRayReconstructionSettings"))
	{
		RenderingSettingsPanelUi::DrawComboOptionRow(
		    "##RayReconstructionMode",
		    "Mode",
		    settings.RayReconstructionMode,
		    rayReconstructionModeOptions,
		    [&settingsSection](EngineRayReconstructionMode value) { settingsSection.SetRayReconstructionMode(value); });
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
