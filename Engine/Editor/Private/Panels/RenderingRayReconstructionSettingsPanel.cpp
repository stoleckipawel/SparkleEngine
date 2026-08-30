#include "PCH.h"

#include "Panels/RenderingRayReconstructionSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <imgui.h>

void DrawRayReconstructionSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	using namespace RenderingSettingsPanelUi;

	static constexpr ComboOption<EngineRayReconstructionMode> rayReconstructionModeOptions[] = {
	    {"Off", EngineRayReconstructionMode::Off},
	    {"NVIDIA DLSS Ray Reconstruction", EngineRayReconstructionMode::NvidiaDlssRayReconstruction},
	};

	if (!MatchesFilter(filterText, "Ray Reconstruction", "dlss ray reconstruction indirect specular diffuse")
	    || !BeginSettingsCategory("Ray Reconstruction"))
	{
		return;
	}

	if (BeginSettingsTable("##RenderingRayReconstructionSettings"))
	{
		DrawComboOptionRow(
		    "##RayReconstructionMode",
		    "Mode",
		    settings.RayReconstructionMode,
		    rayReconstructionModeOptions,
		    [&settingsSection](EngineRayReconstructionMode value) { settingsSection.SetRayReconstructionMode(value); });
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
