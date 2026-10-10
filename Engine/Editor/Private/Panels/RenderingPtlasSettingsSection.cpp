#include "PCH.h"

#include "Panels/RenderingPtlasSettingsSection.h"

#include "Panels/RenderingSettingsUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsController.h"

#include <imgui.h>

void DrawPtlasSettingsSection(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings)
{
	static constexpr RenderingSettingsUi::ComboOption<RayTracingPtlasPartitionUpdateMode> partitionUpdateModeOptions[] = {
	    {"Always update partition", RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition},
	    {"Always move dynamic to global", RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal},
	    {"Update partition nearby, move to global otherwise", RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise},
	};

	if (ImGui::TreeNodeEx("PTLAS", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (RenderingSettingsUi::BeginSettingsTable("##RenderingPtlasSettings"))
		{
			RenderingSettingsUi::DrawBooleanRow("##PtlasActive", "Enabled", settings.PtlasActive, [&settingsController](bool value) { settingsController.SetPtlasActive(value); });
			if (settings.PtlasActive)
			{
				RenderingSettingsUi::DrawUnsignedIntSliderRow(
				    "##PtlasPartitionsPerAxis",
				    "Partitions per axis",
				    settings.PtlasPartitionsPerAxis,
				    1u,
				    64u,
				    [&settingsController](std::uint32_t value) { settingsController.SetPtlasPartitionsPerAxis(value); });

				RenderingSettingsUi::DrawComboOptionRow(
				    "##PtlasPartitionUpdateMode",
				    "Partition update mode",
				    settings.PtlasPartitionUpdateMode,
				    partitionUpdateModeOptions,
				    [&settingsController](RayTracingPtlasPartitionUpdateMode value) { settingsController.SetPtlasPartitionUpdateMode(value); });

				RenderingSettingsUi::DrawBooleanRow(
				    "##PtlasMarkAllDynamicInPartition",
				    "Mark all dynamic in partition",
				    settings.PtlasMarkAllDynamicInPartition,
				    [&settingsController](bool value) { settingsController.SetPtlasMarkAllDynamicInPartition(value); });

				RenderingSettingsUi::DrawFloatInputRow(
				    "##PtlasModeChangeDistance",
				    "Mode change distance",
				    settings.PtlasModeChangeDistance,
				    [&settingsController](float value) { settingsController.SetPtlasModeChangeDistance(value); });
			}
			ImGui::EndTable();
		}
		ImGui::TreePop();
	}
}
