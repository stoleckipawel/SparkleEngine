#include "PCH.h"

#include "Panels/RenderingDisplaySettingsPanel.h"

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
	static constexpr ComboOption<EngineExposureMode> exposureModeOptions[] = {
	    {"Manual", EngineExposureMode::Manual},
	    {"Automatic", EngineExposureMode::Automatic},
	};
	static constexpr ComboOption<EngineExposureMeteringMethod> exposureMeteringMethodOptions[] = {
	    {"Parallel reduction", EngineExposureMeteringMethod::ParallelReduction},
	    {"Downsample pyramid", EngineExposureMeteringMethod::DownsamplePyramid},
	};
	static constexpr ComboOption<EngineOutputColorEncoding> outputColorEncodingOptions[] = {
	    {"Automatic", EngineOutputColorEncoding::Automatic},
	    {"Linear", EngineOutputColorEncoding::Linear},
	    {"sRGB", EngineOutputColorEncoding::Srgb},
	};

	if (!MatchesFilter(
	        filterText,
	        "Display",
	        "display vsync high-performance adapter gpu back buffer format tone mapper aces reinhard exposure automatic manual metering reduction downsample pyramid compensation luminance sdr srgb output encoding") ||
	    !BeginSettingsCategory("Display"))
	{
		return;
	}

	if (BeginSettingsTable("##RenderingDisplaySettings"))
	{
		DrawBooleanRow(
		    "##VSync",
		    "VSync",
		    settings.VSync,
		    [&settingsSection](bool value) { settingsSection.SetVSync(value); });
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
		    "##ExposureMode",
		    "Exposure mode",
		    settings.ExposureMode,
		    exposureModeOptions,
		    [&settingsSection](EngineExposureMode value) { settingsSection.SetExposureMode(value); });
		DrawComboOptionRow(
		    "##ExposureMeteringMethod",
		    "Exposure metering",
		    settings.ExposureMeteringMethod,
		    exposureMeteringMethodOptions,
		    [&settingsSection](EngineExposureMeteringMethod value) { settingsSection.SetExposureMeteringMethod(value); });
		DrawComboOptionRow(
		    "##OutputColorEncoding",
		    "Output encoding",
		    settings.OutputColorEncoding,
		    outputColorEncodingOptions,
		    [&settingsSection](EngineOutputColorEncoding value) { settingsSection.SetOutputColorEncoding(value); });
		ImGui::BeginDisabled(settings.ExposureMode != EngineExposureMode::Manual);
		DrawFloatInputRow(
		    "##ManualExposure",
		    "Manual exposure",
		    settings.ManualExposure,
		    [&settingsSection](float value) { settingsSection.SetManualExposure(value); },
		    0.1f,
		    1.0f,
		    "%.4f");
		ImGui::EndDisabled();
		DrawFloatInputRow(
		    "##ExposureCompensation",
		    "Exposure compensation EV",
		    settings.ExposureCompensation,
		    [&settingsSection](float value) { settingsSection.SetExposureCompensation(value); },
		    0.1f,
		    1.0f,
		    "%.2f");
		DrawFloatInputRow(
		    "##ExposureTargetLuminance",
		    "Target luminance",
		    settings.ExposureTargetLuminance,
		    [&settingsSection](float value) { settingsSection.SetExposureTargetLuminance(value); },
		    0.01f,
		    0.1f,
		    "%.4f");
		DrawFloatInputRow(
		    "##ExposureMin",
		    "Min exposure",
		    settings.ExposureMin,
		    [&settingsSection](float value) { settingsSection.SetExposureMin(value); },
		    0.0001f,
		    0.01f,
		    "%.6f");
		DrawFloatInputRow(
		    "##ExposureMax",
		    "Max exposure",
		    settings.ExposureMax,
		    [&settingsSection](float value) { settingsSection.SetExposureMax(value); },
		    1.0f,
		    64.0f,
		    "%.3f");
		DrawFloatInputRow(
		    "##ExposureAdaptationSpeedUp",
		    "Adapt speed up",
		    settings.ExposureAdaptationSpeedUp,
		    [&settingsSection](float value) { settingsSection.SetExposureAdaptationSpeedUp(value); },
		    0.1f,
		    1.0f,
		    "%.3f");
		DrawFloatInputRow(
		    "##ExposureAdaptationSpeedDown",
		    "Adapt speed down",
		    settings.ExposureAdaptationSpeedDown,
		    [&settingsSection](float value) { settingsSection.SetExposureAdaptationSpeedDown(value); },
		    0.1f,
		    1.0f,
		    "%.3f");
		ImGui::EndTable();
	}
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}
