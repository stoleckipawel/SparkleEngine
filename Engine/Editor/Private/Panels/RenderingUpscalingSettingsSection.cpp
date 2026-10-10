#include "PCH.h"

#include "Panels/RenderingUpscalingSettingsSection.h"

#include "Panels/RenderingSettingsUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsController.h"

#include <imgui.h>

void DrawUpscalingSettingsSection(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings, const char* filterText)
{
	static constexpr RenderingSettingsUi::ComboOption<EUpscalerProviderKind> upscalerProviderOptions[] = {
	    {"Linear", EUpscalerProviderKind::Linear},
	    {"NVIDIA DLSS", EUpscalerProviderKind::NvidiaDlss},
	};
	static constexpr RenderingSettingsUi::ComboOption<EUpscalerQualityMode> upscalerQualityOptions[] = {
	    {"Native AA", EUpscalerQualityMode::NativeAA},
	    {"Quality", EUpscalerQualityMode::Quality},
	    {"Balanced", EUpscalerQualityMode::Balanced},
	    {"Performance", EUpscalerQualityMode::Performance},
	    {"Ultra performance", EUpscalerQualityMode::UltraPerformance},
	};

	if (!RenderingSettingsUi::MatchesFilter(filterText, "Upscaling", "upscaler upscaling linear bilinear dlss quality native aa balanced performance")
	    || !RenderingSettingsUi::BeginSettingsCategory("Upscaling"))
	{
		return;
	}

	if (RenderingSettingsUi::BeginSettingsTable("##RenderingUpscalingSettings"))
	{
		RenderingSettingsUi::DrawComboOptionRow(
		    "##UpscalerProvider",
		    "Provider",
		    settings.UpscalerProvider,
		    upscalerProviderOptions,
		    [&settingsController](EUpscalerProviderKind value) { settingsController.SetUpscalerProvider(value); });

		RenderingSettingsUi::DrawComboOptionRow(
		    "##UpscalerQualityMode",
		    "Quality mode",
		    settings.UpscalerQualityMode,
		    upscalerQualityOptions,
		    [&settingsController](EUpscalerQualityMode value) { settingsController.SetUpscalerQualityMode(value); });

		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
