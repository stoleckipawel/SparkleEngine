#include "PCH.h"

#include "Panels/RenderingDisplaySettingsPanel.h"

#include "Panels/ExposureSettingsEditor.h"
#include "Panels/RenderingSettingsPanelUi.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <imgui.h>

void DrawDisplaySettingsSection(
    EngineRenderingSettingsSection& settingsSection,
    const EngineRenderingSettingsState& settings,
    const char* filterText)
{
	using namespace RenderingSettingsPanelUi;

	static constexpr ComboOption<PixelFormat> backBufferFormatOptions[] = {
	    {"R8G8B8A8 UNorm", PixelFormat::R8G8B8A8_UNorm},
	    {"R8G8B8A8 sRGB", PixelFormat::R8G8B8A8_UNorm_Srgb},
	    {"B8G8R8A8 UNorm", PixelFormat::B8G8R8A8_UNorm},
	    {"B8G8R8A8 sRGB", PixelFormat::B8G8R8A8_UNorm_Srgb},
	};
	static constexpr ComboOption<EngineToneMapper> toneMapperOptions[] = {
	    {"Reinhard", EngineToneMapper::Reinhard},
	    {"ACES approximate", EngineToneMapper::AcesApprox},
	    {"ACES fitted filmic", EngineToneMapper::AcesFilmic},
	};
	static constexpr ComboOption<EngineOutputColorEncoding> outputColorEncodingOptions[] = {
	    {"Automatic", EngineOutputColorEncoding::Automatic},
	    {"Linear", EngineOutputColorEncoding::Linear},
	    {"sRGB", EngineOutputColorEncoding::Srgb},
	};

	if (!MatchesFilter(
	        filterText,
	        "Display",
	        "display vsync high-performance adapter gpu back buffer format tone mapper aces reinhard exposure automatic manual metering "
	        "reduction downsample pyramid compensation luminance sdr srgb output encoding")
	    || !BeginSettingsCategory("Display"))
	{
		return;
	}

	if (BeginSettingsTable("##RenderingDisplaySettings"))
	{
		DrawBooleanRow("##VSync", "VSync", settings.VSync, [&settingsSection](bool value) { settingsSection.SetVSync(value); });
		DrawComboOptionRow(
		    "##BackBufferFormat",
		    "Back buffer format",
		    settings.BackBufferFormat,
		    backBufferFormatOptions,
		    [&settingsSection](PixelFormat value) { settingsSection.SetBackBufferFormat(value); });
		DrawBooleanRow(
		    "##PreferHighPerformanceAdapter",
		    "Prefer high-performance adapter",
		    settings.PreferHighPerformanceAdapter,
		    [&settingsSection](bool value) { settingsSection.SetPreferHighPerformanceAdapter(value); });
		DrawComboOptionRow(
		    "##ToneMapper",
		    "Tone mapper",
		    settings.ToneMapper,
		    toneMapperOptions,
		    [&settingsSection](EngineToneMapper value) { settingsSection.SetToneMapper(value); });
		DrawComboOptionRow(
		    "##OutputColorEncoding",
		    "Output encoding",
		    settings.OutputColorEncoding,
		    outputColorEncodingOptions,
		    [&settingsSection](EngineOutputColorEncoding value) { settingsSection.SetOutputColorEncoding(value); });
		ExposureSettingsEditor::DrawSettings(settingsSection, settings);
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
