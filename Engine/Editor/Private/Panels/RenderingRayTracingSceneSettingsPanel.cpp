#include "PCH.h"

#include "Panels/RenderingRayTracingSceneSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <imgui.h>

void DrawRayTracingSceneSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	using namespace RenderingSettingsPanelUi;

	static constexpr ComboOption<RayTracingPtlasPartitionUpdateMode> partitionUpdateModeOptions[] = {
	    {"Always update partition", RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition},
	    {"Always move dynamic to global", RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal},
	    {"Update partition nearby, move to global otherwise",
	        RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise},
	};

	if (!MatchesFilter(
	        filterText,
	        "Ray Tracing Scene",
	        "ray tracing scene tlas refit ptlas active partition update mode partitions dynamic distance acceleration structure")
	    || !BeginSettingsCategory("Ray Tracing Scene"))
	{
		return;
	}

	if (BeginSettingsTable("##RenderingRayTracingSceneSettings"))
	{
		DrawBooleanRow(
		    "##PtlasActive",
		    "PTLAS Active",
		    settings.PtlasActive,
		    [&settingsSection](bool value) { settingsSection.SetPtlasActive(value); });
		ImGui::BeginDisabled(settings.PtlasActive);
		DrawBooleanRow(
		    "##RefitTlas",
		    "Refit TLAS",
		    settings.RefitTlas,
		    [&settingsSection](bool value) { settingsSection.SetRefitTlas(value); });
		ImGui::EndDisabled();
		DrawUnsignedIntSliderRow(
		    "##PtlasPartitionsPerAxis",
		    "PTLAS partitions per axis",
		    settings.PtlasPartitionsPerAxis,
		    1u,
		    64u,
		    [&settingsSection](std::uint32_t value) { settingsSection.SetPtlasPartitionsPerAxis(value); });
		DrawComboOptionRow(
		    "##PtlasPartitionUpdateMode",
		    "Partition update mode",
		    settings.PtlasPartitionUpdateMode,
		    partitionUpdateModeOptions,
		    [&settingsSection](RayTracingPtlasPartitionUpdateMode value) { settingsSection.SetPtlasPartitionUpdateMode(value); });
		DrawBooleanRow(
		    "##PtlasMarkAllDynamicInPartition",
		    "Mark all dynamic in partition",
		    settings.PtlasMarkAllDynamicInPartition,
		    [&settingsSection](bool value) { settingsSection.SetPtlasMarkAllDynamicInPartition(value); });
		DrawFloatInputRow(
		    "##PtlasModeChangeDistance",
		    "Mode change distance",
		    settings.PtlasModeChangeDistance,
		    [&settingsSection](float value) { settingsSection.SetPtlasModeChangeDistance(value); });
		ImGui::EndTable();
	}
}
