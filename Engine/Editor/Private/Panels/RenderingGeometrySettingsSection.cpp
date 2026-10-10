#include "PCH.h"

#include "Panels/RenderingGeometrySettingsSection.h"

#include "Panels/RenderingSettingsUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsController.h"

#include <imgui.h>

void DrawGeometrySettingsSection(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings, const char* filterText)
{
	static constexpr RenderingSettingsUi::ComboOption<GBufferAlgorithm> gBufferAlgorithmOptions[] = {{"Rasterized", GBufferAlgorithm::Rasterized}, {"Ray tracing", GBufferAlgorithm::RayTracing}};
	if (RenderingSettingsUi::MatchesFilter(filterText, "Geometry", "geometry mesh auto batching") && RenderingSettingsUi::BeginSettingsCategory("Geometry"))
	{
		if (RenderingSettingsUi::BeginSettingsTable("##RenderingGeometrySettings"))
		{
			RenderingSettingsUi::DrawComboOptionRow(
			    "##GBufferAlgorithm",
			    "GBuffer algorithm",
			    settings.SelectedGBufferAlgorithm,
			    gBufferAlgorithmOptions,
			    [&settingsController](GBufferAlgorithm value) { settingsController.SetGBufferAlgorithm(value); });

			RenderingSettingsUi::DrawBooleanRow(
			    "##MeshAutoBatching",
			    "Mesh auto batching",
			    settings.MeshAutoBatching,
			    [&settingsController](bool value) { settingsController.SetMeshAutoBatching(value); });

			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}
}
