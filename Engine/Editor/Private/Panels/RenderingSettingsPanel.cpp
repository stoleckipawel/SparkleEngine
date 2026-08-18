#include "PCH.h"

#include "Panels/RenderingSettingsPanel.h"

#include "Panels/RenderingDisplaySettingsPanel.h"
#include "Panels/RenderingRayReconstructionSettingsPanel.h"
#include "Panels/RenderingRayTracingSceneSettingsPanel.h"
#include "Panels/RenderingSettingsPanelUi.h"
#include "Panels/RenderingUpscalingSettingsPanel.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Style/SparkleUiPalette.h"

#include <imgui.h>

#include <string>

void RenderingSettingsPanel::SetSettings(EngineRenderingSettingsSection* settings) noexcept
{
	m_settings = settings;
}

void RenderingSettingsPanel::RefreshFromRuntimeState() noexcept
{
	if (m_settings != nullptr)
	{
		m_settings->RefreshFromRuntimeState();
	}
}

bool RenderingSettingsPanel::HasPendingRestart() const noexcept
{
	return m_settings != nullptr && m_settings->HasPendingRestart();
}

void RenderingSettingsPanel::BuildUI(bool disableInteraction, const char* filterText)
{
	if (m_settings == nullptr)
	{
		return;
	}

	const EngineRenderingSettingsState& settings = m_settings->GetState();

	ImGui::TextUnformatted("Engine - Rendering");
	ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
	ImGui::TextUnformatted("Rendering settings.");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0.0f, 2.0f));

	if (HasPendingRestart())
	{
		const std::string restartMessage = m_settings->BuildPendingRestartMessage();
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::AccentStrong());
		ImGui::TextWrapped("%s", restartMessage.c_str());
		ImGui::PopStyleColor();
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	ImGui::BeginDisabled(disableInteraction);
	using namespace RenderingSettingsPanelUi;
	static constexpr ComboOption<GBufferMode> gBufferModeOptions[] = {
	    {"Rasterized", GBufferMode::Rasterized},
	    {"Ray traced", GBufferMode::Raytraced}};
	static constexpr ComboOption<LightingMode> lightingModeOptions[] = {
	    {"ReSTIR real-time path tracing", LightingMode::RestirPathTraced},
	    {"Convergent reference path tracing", LightingMode::ReferencePathTraced}};

	DrawDisplaySettingsSection(*m_settings, settings, filterText);

	if (MatchesFilter(filterText, "Geometry", "geometry mesh auto batching") && BeginSettingsCategory("Geometry"))
	{
		if (BeginSettingsTable("##RenderingGeometrySettings"))
		{
			DrawComboOptionRow(
			    "##GBufferMode",
			    "GBuffer mode",
			    settings.GBuffer,
			    gBufferModeOptions,
			    [this](GBufferMode value)
			    {
				    m_settings->SetGBufferMode(value);
			    });
			DrawBooleanRow(
			    "##MeshAutoBatching",
			    "Mesh auto batching",
			    settings.MeshAutoBatching,
			    [this](bool value)
			    {
				    m_settings->SetMeshAutoBatching(value);
			    });
			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	if (MatchesFilter(filterText, "Lighting", "lighting mode") && BeginSettingsCategory("Lighting"))
	{
		if (BeginSettingsTable("##RenderingLightingSettings"))
		{
			DrawComboOptionRow(
			    "##LightingMode",
			    "Lighting mode",
			    settings.Lighting,
			    lightingModeOptions,
			    [this](LightingMode value)
			    {
				    m_settings->SetLightingMode(value);
			    });
			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	DrawRayReconstructionSettingsSection(*m_settings, settings, filterText);
	DrawUpscalingSettingsSection(*m_settings, settings, filterText);
	DrawRayTracingSceneSettingsSection(*m_settings, settings, filterText);

	ImGui::EndDisabled();
}
