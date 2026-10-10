#include "PCH.h"

#include "Panels/RenderingDisplaySettingsSection.h"

#include "Panels/RenderingSettingsUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Settings/EngineRenderingSettingsController.h"

#include <imgui.h>

void DrawDisplaySettingsSection(EngineRenderingSettingsController& settingsController, const EngineRenderingSettingsState& settings, const char* filterText)
{
	static constexpr RenderingSettingsUi::ComboOption<PixelFormat> backBufferFormatOptions[] = {
	    {"R8G8B8A8 UNorm", PixelFormat::R8G8B8A8_UNorm},
	    {"R8G8B8A8 sRGB", PixelFormat::R8G8B8A8_UNorm_Srgb},
	    {"B8G8R8A8 UNorm", PixelFormat::B8G8R8A8_UNorm},
	    {"B8G8R8A8 sRGB", PixelFormat::B8G8R8A8_UNorm_Srgb},
	};

	static constexpr RenderingSettingsUi::ComboOption<EngineOutputColorEncoding> outputColorEncodingOptions[] = {
	    {"Automatic", EngineOutputColorEncoding::Automatic},
	    {"Linear", EngineOutputColorEncoding::Linear},
	    {"sRGB", EngineOutputColorEncoding::Srgb},
	};

	if (!RenderingSettingsUi::MatchesFilter(filterText, "Display", "display vsync high-performance adapter gpu back buffer format sdr srgb linear output encoding")
	    || !RenderingSettingsUi::BeginSettingsCategory("Display"))
	{
		return;
	}

	if (RenderingSettingsUi::BeginSettingsTable("##RenderingDisplaySettings"))
	{
		RenderingSettingsUi::DrawBooleanRow("##VSync", "VSync", settings.VSync, [&settingsController](bool value) { settingsController.SetVSync(value); });

		RenderingSettingsUi::DrawComboOptionRow(
		    "##BackBufferFormat",
		    "Back buffer format",
		    settings.BackBufferFormat,
		    backBufferFormatOptions,
		    [&settingsController](PixelFormat value) { settingsController.SetBackBufferFormat(value); });

		RenderingSettingsUi::DrawBooleanRow(
		    "##PreferHighPerformanceAdapter",
		    "Prefer high-performance adapter",
		    settings.PreferHighPerformanceAdapter,
		    [&settingsController](bool value) { settingsController.SetPreferHighPerformanceAdapter(value); });

		RenderingSettingsUi::DrawComboOptionRow(
		    "##OutputColorEncoding",
		    "Output encoding",
		    settings.OutputColorEncoding,
		    outputColorEncodingOptions,
		    [&settingsController](EngineOutputColorEncoding value) { settingsController.SetOutputColorEncoding(value); });

		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
