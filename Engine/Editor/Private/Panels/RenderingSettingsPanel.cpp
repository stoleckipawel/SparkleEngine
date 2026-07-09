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

	DrawDisplaySettingsSection(*m_settings, settings, filterText);

	if (MatchesFilter(filterText, "Geometry", "geometry mesh auto batching") && BeginSettingsCategory("Geometry"))
	{
		if (BeginSettingsTable("##RenderingGeometrySettings"))
		{
			DrawBooleanRow(
			    "##MeshAutoBatching",
			    "Mesh auto batching",
			    settings.MeshAutoBatching,
			    [this](bool value) { m_settings->SetMeshAutoBatching(value); });
			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	if (MatchesFilter(filterText, "Lighting", "lighting light budget directional point spot rect area indirect diffuse specular reflection bounces") &&
	    BeginSettingsCategory("Lighting"))
	{
		if (BeginSettingsTable("##RenderingLightingSettings"))
		{
			DrawUnsignedIntInputRow(
			    "##MaxDirectionalLights",
			    "Max directional lights",
			    settings.MaxDirectionalLights,
			    [this](std::uint32_t value) { m_settings->SetMaxDirectionalLights(value); });
			DrawUnsignedIntInputRow(
			    "##MaxPointLights",
			    "Max point lights",
			    settings.MaxPointLights,
			    [this](std::uint32_t value) { m_settings->SetMaxPointLights(value); });
			DrawUnsignedIntInputRow(
			    "##MaxSpotLights",
			    "Max spot lights",
			    settings.MaxSpotLights,
			    [this](std::uint32_t value) { m_settings->SetMaxSpotLights(value); });
			DrawUnsignedIntInputRow(
			    "##MaxRectLights",
			    "Max rect lights",
			    settings.MaxRectLights,
			    [this](std::uint32_t value) { m_settings->SetMaxRectLights(value); });
			DrawUnsignedIntSliderRow(
			    "##IndirectDiffuseBounceCount",
			    "Indirect diffuse bounces",
			    settings.IndirectDiffuseBounceCount,
			    1u,
			    8u,
			    [this](std::uint32_t value) { m_settings->SetIndirectDiffuseBounceCount(value); });
			DrawUnsignedIntSliderRow(
			    "##IndirectSpecularBounceCount",
			    "Indirect specular bounces",
			    settings.IndirectSpecularBounceCount,
			    1u,
			    8u,
			    [this](std::uint32_t value) { m_settings->SetIndirectSpecularBounceCount(value); });
			ImGui::EndTable();
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	DrawRayReconstructionSettingsSection(*m_settings, settings, filterText);
	DrawUpscalingSettingsSection(*m_settings, settings, filterText);
	DrawRayTracingSceneSettingsSection(*m_settings, settings, filterText);

	ImGui::EndDisabled();
}
