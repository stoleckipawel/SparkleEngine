#include "PCH.h"

#include "Panels/RenderingUpscalingSettingsPanel.h"

#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <imgui.h>

namespace
{
	using namespace RenderingSettingsPanelUi;

	int ToUpscalerProviderIndex(EngineUpscalerProvider provider) noexcept
	{
		return provider == EngineUpscalerProvider::NvidiaDlss ? 1 : 0;
	}

	EngineUpscalerProvider FromUpscalerProviderIndex(int index) noexcept
	{
		return index == 1 ? EngineUpscalerProvider::NvidiaDlss : EngineUpscalerProvider::Linear;
	}

	int ToUpscalerQualityModeIndex(EngineUpscalerQualityMode mode) noexcept
	{
		switch (mode)
		{
			case EngineUpscalerQualityMode::Quality:
				return 1;
			case EngineUpscalerQualityMode::Balanced:
				return 2;
			case EngineUpscalerQualityMode::Performance:
				return 3;
			case EngineUpscalerQualityMode::UltraPerformance:
				return 4;
			case EngineUpscalerQualityMode::NativeAA:
			default:
				return 0;
		}
	}

	EngineUpscalerQualityMode FromUpscalerQualityModeIndex(int index) noexcept
	{
		switch (index)
		{
			case 1:
				return EngineUpscalerQualityMode::Quality;
			case 2:
				return EngineUpscalerQualityMode::Balanced;
			case 3:
				return EngineUpscalerQualityMode::Performance;
			case 4:
				return EngineUpscalerQualityMode::UltraPerformance;
			case 0:
			default:
				return EngineUpscalerQualityMode::NativeAA;
		}
	}
}

void DrawUpscalingSettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	static constexpr const char* upscalerProviderLabels[] = {
	    "Linear",
	    "NVIDIA DLSS",
	};
	static constexpr const char* upscalerQualityLabels[] = {
	    "Native AA",
	    "Quality",
	    "Balanced",
	    "Performance",
	    "Ultra performance",
	};
	if (!RenderingSettingsPanelUi::MatchesFilter(
	        filterText,
	        "Upscaling",
	        "upscaler upscaling linear bilinear dlss quality native aa balanced performance") ||
	    !RenderingSettingsPanelUi::BeginSettingsCategory("Upscaling"))
	{
		return;
	}

	if (RenderingSettingsPanelUi::BeginSettingsTable("##RenderingUpscalingSettings"))
	{
		RenderingSettingsPanelUi::DrawComboRow(
		    "##UpscalerProvider",
		    "Provider",
		    ToUpscalerProviderIndex(settings.UpscalerProvider),
		    upscalerProviderLabels,
		    IM_ARRAYSIZE(upscalerProviderLabels),
		    [&settingsSection](int value) { settingsSection.SetUpscalerProvider(FromUpscalerProviderIndex(value)); });
		RenderingSettingsPanelUi::DrawComboRow(
		    "##UpscalerQualityMode",
		    "Quality mode",
		    ToUpscalerQualityModeIndex(settings.UpscalerQualityMode),
		    upscalerQualityLabels,
		    IM_ARRAYSIZE(upscalerQualityLabels),
		    [&settingsSection](int value) { settingsSection.SetUpscalerQualityMode(FromUpscalerQualityModeIndex(value)); });
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
