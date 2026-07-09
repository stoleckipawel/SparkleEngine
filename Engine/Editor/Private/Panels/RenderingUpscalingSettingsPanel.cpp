#include "PCH.h"

#include "Panels/RenderingUpscalingSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <imgui.h>

void DrawUpscalingSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	using namespace RenderingSettingsPanelUi;

	static constexpr ComboOption<EUpscalerProviderKind> upscalerProviderOptions[] = {
	    {"Linear", EUpscalerProviderKind::Linear},
	    {"NVIDIA DLSS", EUpscalerProviderKind::NvidiaDlss},
	};
	static constexpr ComboOption<EUpscalerQualityMode> upscalerQualityOptions[] = {
	    {"Native AA", EUpscalerQualityMode::NativeAA},
	    {"Quality", EUpscalerQualityMode::Quality},
	    {"Balanced", EUpscalerQualityMode::Balanced},
	    {"Performance", EUpscalerQualityMode::Performance},
	    {"Ultra performance", EUpscalerQualityMode::UltraPerformance},
	};
	if (!MatchesFilter(
	        filterText,
	        "Upscaling",
	        "upscaler upscaling linear bilinear dlss quality native aa balanced performance") ||
	    !BeginSettingsCategory("Upscaling"))
	{
		return;
	}

	if (BeginSettingsTable("##RenderingUpscalingSettings"))
	{
		DrawComboOptionRow(
		    "##UpscalerProvider",
		    "Provider",
		    settings.UpscalerProvider,
		    upscalerProviderOptions,
		    [&settingsSection](EUpscalerProviderKind value) { settingsSection.SetUpscalerProvider(value); });
		DrawComboOptionRow(
		    "##UpscalerQualityMode",
		    "Quality mode",
		    settings.UpscalerQualityMode,
		    upscalerQualityOptions,
		    [&settingsSection](EUpscalerQualityMode value) { settingsSection.SetUpscalerQualityMode(value); });
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
