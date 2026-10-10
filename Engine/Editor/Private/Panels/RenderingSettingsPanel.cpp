#include "PCH.h"

#include "Panels/RenderingSettingsPanel.h"

#include "Panels/RenderingDisplaySettingsSection.h"
#include "Panels/RenderingExposureSettingsSection.h"
#include "Panels/RenderingGeometrySettingsSection.h"
#include "Panels/RenderingRayReconstructionSettingsSection.h"
#include "Panels/RenderingRayTracingSceneSettingsSection.h"
#include "Panels/RenderingToneMappingSettingsSection.h"
#include "Panels/RenderingUpscalingSettingsSection.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsController.h"
#include "Style/SparkleUiPalette.h"
#if SPARKLE_TARGET_EDITOR && !SPARKLE_BUILD_SHIPPING
  #include "Viewport/ExternalCapture/ExternalCaptureSettingsSection.h"
#endif

#include <imgui.h>

#include <string>

void RenderingSettingsPanel::SetSettings(EngineRenderingSettingsController* settings) noexcept
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
	DrawDisplaySettingsSection(*m_settings, settings, filterText);
	DrawExposureSettingsSection(*m_settings, settings, filterText);
	DrawToneMappingSettingsSection(*m_settings, settings, filterText);

	DrawGeometrySettingsSection(*m_settings, settings, filterText);

	DrawRayReconstructionSettingsSection(*m_settings, settings, filterText);
	DrawUpscalingSettingsSection(*m_settings, settings, filterText);
	DrawRayTracingSceneSettingsSection(*m_settings, settings, filterText);
#if SPARKLE_TARGET_EDITOR && !SPARKLE_BUILD_SHIPPING
	DrawExternalCaptureSettingsSection(*m_settings, filterText);
#endif

	ImGui::EndDisabled();
}
