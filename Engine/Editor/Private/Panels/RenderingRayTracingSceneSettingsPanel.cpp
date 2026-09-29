#include "PCH.h"

#include "Panels/RenderingRayTracingSceneSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsSection.h"

#include <imgui.h>

void DrawRayTracingSceneSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	static constexpr RenderingSettingsPanelUi::ComboOption<RayTracingPtlasPartitionUpdateMode> partitionUpdateModeOptions[] = {
	    {"Always update partition", RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition},
	    {"Always move dynamic to global", RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal},
	    {"Update partition nearby, move to global otherwise",
	        RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise},
	};

	if (!RenderingSettingsPanelUi::MatchesFilter(
	        filterText,
	        "Ray Tracing Scene",
	        "ray tracing scene tlas refit ptlas active partition update mode partitions dynamic distance acceleration structure")
	    || !RenderingSettingsPanelUi::BeginSettingsCategory("Ray Tracing Scene"))
	{
		return;
	}

	if (RenderingSettingsPanelUi::BeginSettingsTable("##RenderingRayTracingSceneSettings"))
	{
		RenderingSettingsPanelUi::DrawBooleanRow(
		    "##PtlasActive",
		    "PTLAS Active",
		    settings.PtlasActive,
		    [&settingsSection](bool value) { settingsSection.SetPtlasActive(value); });
		ImGui::BeginDisabled(settings.PtlasActive);
		RenderingSettingsPanelUi::DrawBooleanRow(
		    "##RefitTlas",
		    "Refit TLAS",
		    settings.RefitTlas,
		    [&settingsSection](bool value) { settingsSection.SetRefitTlas(value); });
		ImGui::EndDisabled();
		RenderingSettingsPanelUi::DrawUnsignedIntSliderRow(
		    "##PtlasPartitionsPerAxis",
		    "PTLAS partitions per axis",
		    settings.PtlasPartitionsPerAxis,
		    1u,
		    64u,
		    [&settingsSection](std::uint32_t value) { settingsSection.SetPtlasPartitionsPerAxis(value); });
		RenderingSettingsPanelUi::DrawComboOptionRow(
		    "##PtlasPartitionUpdateMode",
		    "Partition update mode",
		    settings.PtlasPartitionUpdateMode,
		    partitionUpdateModeOptions,
		    [&settingsSection](RayTracingPtlasPartitionUpdateMode value) { settingsSection.SetPtlasPartitionUpdateMode(value); });
		RenderingSettingsPanelUi::DrawBooleanRow(
		    "##PtlasMarkAllDynamicInPartition",
		    "Mark all dynamic in partition",
		    settings.PtlasMarkAllDynamicInPartition,
		    [&settingsSection](bool value) { settingsSection.SetPtlasMarkAllDynamicInPartition(value); });
		RenderingSettingsPanelUi::DrawFloatInputRow(
		    "##PtlasModeChangeDistance",
		    "Mode change distance",
		    settings.PtlasModeChangeDistance,
		    [&settingsSection](float value) { settingsSection.SetPtlasModeChangeDistance(value); });
		ImGui::EndTable();
	}
}
