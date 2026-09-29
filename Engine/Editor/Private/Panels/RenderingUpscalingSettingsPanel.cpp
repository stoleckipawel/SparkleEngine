#include "PCH.h"

#include "Panels/RenderingUpscalingSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsSection.h"

#include <imgui.h>

void DrawUpscalingSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	static constexpr RenderingSettingsPanelUi::ComboOption<EUpscalerProviderKind> upscalerProviderOptions[] = {
	    {"Linear", EUpscalerProviderKind::Linear},
	    {"NVIDIA DLSS", EUpscalerProviderKind::NvidiaDlss},
	};
	static constexpr RenderingSettingsPanelUi::ComboOption<EUpscalerQualityMode> upscalerQualityOptions[] = {
	    {"Native AA", EUpscalerQualityMode::NativeAA},
	    {"Quality", EUpscalerQualityMode::Quality},
	    {"Balanced", EUpscalerQualityMode::Balanced},
	    {"Performance", EUpscalerQualityMode::Performance},
	    {"Ultra performance", EUpscalerQualityMode::UltraPerformance},
	};
	if (!RenderingSettingsPanelUi::MatchesFilter(
	        filterText,
	        "Upscaling",
	        "upscaler upscaling linear bilinear dlss quality native aa balanced performance")
	    || !RenderingSettingsPanelUi::BeginSettingsCategory("Upscaling"))
	{
		return;
	}

	if (RenderingSettingsPanelUi::BeginSettingsTable("##RenderingUpscalingSettings"))
	{
		RenderingSettingsPanelUi::DrawComboOptionRow(
		    "##UpscalerProvider",
		    "Provider",
		    settings.UpscalerProvider,
		    upscalerProviderOptions,
		    [&settingsSection](EUpscalerProviderKind value) { settingsSection.SetUpscalerProvider(value); });
		RenderingSettingsPanelUi::DrawComboOptionRow(
		    "##UpscalerQualityMode",
		    "Quality mode",
		    settings.UpscalerQualityMode,
		    upscalerQualityOptions,
		    [&settingsSection](EUpscalerQualityMode value) { settingsSection.SetUpscalerQualityMode(value); });
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
