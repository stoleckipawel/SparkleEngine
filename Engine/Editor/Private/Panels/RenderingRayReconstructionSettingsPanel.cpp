#include "PCH.h"

#include "Panels/RenderingRayReconstructionSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <imgui.h>

namespace
{
	int ToRayReconstructionModeIndex(EngineRayReconstructionMode mode) noexcept
	{
		return mode == EngineRayReconstructionMode::NvidiaDlrr ? 1 : 0;
	}

	EngineRayReconstructionMode FromRayReconstructionModeIndex(int index) noexcept
	{
		return index == 1 ? EngineRayReconstructionMode::NvidiaDlrr : EngineRayReconstructionMode::Off;
	}
}

void DrawRayReconstructionSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	using namespace RenderingSettingsPanelUi;

	static constexpr const char* rayReconstructionModeLabels[] = {
	    "Off",
	    "NVIDIA DLRR",
	};

	if (!MatchesFilter(filterText, "Ray Reconstruction", "dlrr ray reconstruction indirect specular diffuse") ||
	    !BeginSettingsCategory("Ray Reconstruction"))
	{
		return;
	}

	if (BeginSettingsTable("##RenderingRayReconstructionSettings"))
	{
		DrawComboRow(
		    "##RayReconstructionMode",
		    "Mode",
		    ToRayReconstructionModeIndex(settings.RayReconstructionMode),
		    rayReconstructionModeLabels,
		    IM_ARRAYSIZE(rayReconstructionModeLabels),
		    [&settingsSection](int value) { settingsSection.SetRayReconstructionMode(FromRayReconstructionModeIndex(value)); });
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
