#include "../../../PCH.h"
#include "Passes/PostProcessing/Exposure/ExposureSettings.h"

static const auto g_exposureSettingsLogger = Logging::GetOrCreateLogger("Renderer.ExposureSettings");

static std::uint32_t ToShaderExposureMode(EngineExposureMode mode) noexcept
{
	switch (mode)
	{
		case EngineExposureMode::Manual:
			return 0u;
		case EngineExposureMode::Automatic:
			return 1u;
		default:
			Diagnostics::Fatal(g_exposureSettingsLogger, __FILE__, __LINE__, "Exposure settings contain an unknown exposure mode.");
	}
}

ExposureUniformData BuildExposureUniformData(const ResolvedViewportDisplaySettings& settings) noexcept
{
	return ExposureUniformData{
	    .ExposureMode = ToShaderExposureMode(settings.ExposureMode),
	    .ExposureHistoryValid = 0u,
	    .ManualExposure = settings.ManualExposure,
	    .ExposureCompensation = settings.ExposureCompensation,
	    .ExposureTargetLuminance = settings.ExposureTargetLuminance,
	    .ExposureMin = settings.ExposureMin,
	    .ExposureMax = settings.ExposureMax,
	    .ExposureAdaptationSpeedUp = settings.ExposureAdaptationSpeedUp,
	    .ExposureAdaptationSpeedDown = settings.ExposureAdaptationSpeedDown};
}
