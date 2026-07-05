#include "PCH.h"

#include "Panels/RenderingRayReconstructionSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <imgui.h>

namespace
{
	int ToRayReconstructionModeIndex(EngineRayReconstructionMode mode) noexcept
	{
		return mode == EngineRayReconstructionMode::NvidiaDlssRayReconstruction ? 1 : 0;
	}

	EngineRayReconstructionMode FromRayReconstructionModeIndex(int index) noexcept
	{
		return index == 1 ? EngineRayReconstructionMode::NvidiaDlssRayReconstruction : EngineRayReconstructionMode::Off;
	}

	int ToRayReconstructionQualityModeIndex(EngineRayReconstructionQualityMode mode) noexcept
	{
		switch (mode)
		{
			case EngineRayReconstructionQualityMode::Balanced:
				return 1;
			case EngineRayReconstructionQualityMode::Performance:
				return 2;
			case EngineRayReconstructionQualityMode::Quality:
			default:
				return 0;
		}
	}

	EngineRayReconstructionQualityMode FromRayReconstructionQualityModeIndex(int index) noexcept
	{
		switch (index)
		{
			case 1:
				return EngineRayReconstructionQualityMode::Balanced;
			case 2:
				return EngineRayReconstructionQualityMode::Performance;
			case 0:
			default:
				return EngineRayReconstructionQualityMode::Quality;
		}
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
	    "NVIDIA DLSS Ray Reconstruction",
	};
	static constexpr const char* rayReconstructionQualityLabels[] = {
	    "Quality",
	    "Balanced",
	    "Performance",
	};

	if (!MatchesFilter(filterText, "Ray Reconstruction", "dlrr dlss ray reconstruction indirect specular diffuse") ||
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
		ImGui::BeginDisabled(settings.RayReconstructionMode == EngineRayReconstructionMode::Off);
		DrawComboRow(
		    "##RayReconstructionQualityMode",
		    "Quality mode",
		    ToRayReconstructionQualityModeIndex(settings.RayReconstructionQualityMode),
		    rayReconstructionQualityLabels,
		    IM_ARRAYSIZE(rayReconstructionQualityLabels),
		    [&settingsSection](int value) { settingsSection.SetRayReconstructionQualityMode(FromRayReconstructionQualityModeIndex(value)); });
		ImGui::EndDisabled();
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
