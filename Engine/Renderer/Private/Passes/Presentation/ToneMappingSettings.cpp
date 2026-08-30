#include "../../PCH.h"
#include "Passes/Presentation/ToneMappingSettings.h"

static const auto g_toneMappingSettingsLogger = Logging::GetOrCreateLogger("Renderer.ToneMappingSettings");

class ToneMappingValueTranslation final
{
public:
	static std::uint32_t ToShaderToneMapper(EngineToneMapper toneMapper) noexcept
	{
		switch (toneMapper)
		{
			case EngineToneMapper::Reinhard:
				return 0u;
			case EngineToneMapper::AcesApprox:
				return 1u;
			case EngineToneMapper::AcesFilmic:
				return 2u;
			default:
				Diagnostics::Fatal(
				    g_toneMappingSettingsLogger,
				    __FILE__,
				    __LINE__,
				    "Tone-mapping settings contain an unknown tone mapper.");
		}
	}

	static std::uint32_t ToShaderExposureMode(EngineExposureMode mode) noexcept
	{
		switch (mode)
		{
			case EngineExposureMode::Manual:
				return 0u;
			case EngineExposureMode::Automatic:
				return 1u;
			default:
				Diagnostics::Fatal(
				    g_toneMappingSettingsLogger,
				    __FILE__,
				    __LINE__,
				    "Tone-mapping settings contain an unknown exposure mode.");
		}
	}
};

ExposureUniformData BuildExposureUniformData(const ResolvedViewportDisplaySettings& settings, float frameDeltaSeconds) noexcept
{
	return ExposureUniformData{
	    .ExposureMode = ToneMappingValueTranslation::ToShaderExposureMode(settings.ExposureMode),
	    .ExposureHistoryValid = 0u,
	    .FrameDeltaSeconds = frameDeltaSeconds,
	    .ManualExposure = settings.ManualExposure,
	    .ExposureCompensation = settings.ExposureCompensation,
	    .ExposureTargetLuminance = settings.ExposureTargetLuminance,
	    .ExposureMin = settings.ExposureMin,
	    .ExposureMax = settings.ExposureMax,
	    .ExposureAdaptationSpeedUp = settings.ExposureAdaptationSpeedUp,
	    .ExposureAdaptationSpeedDown = settings.ExposureAdaptationSpeedDown};
}

ToneMappingUniformData BuildToneMappingUniformData(EngineToneMapper toneMapper) noexcept
{
	return ToneMappingUniformData{.ToneMapper = ToneMappingValueTranslation::ToShaderToneMapper(toneMapper)};
}
