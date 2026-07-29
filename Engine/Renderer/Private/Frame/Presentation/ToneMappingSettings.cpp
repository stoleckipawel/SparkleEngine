#include "../../PCH.h"
#include "Frame/Presentation/ToneMappingSettings.h"

#include "Frame/Presentation/ToneMappingCVars.h"

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

ExposureUniformData BuildExposureUniformData(float frameDeltaSeconds, bool exposureHistoryValid) noexcept
{
	return ExposureUniformData{
	    .ExposureMode = ToneMappingValueTranslation::ToShaderExposureMode(CVarExposureMode.Get()),
	    .ExposureHistoryValid = exposureHistoryValid ? 1u : 0u,
	    .FrameDeltaSeconds = frameDeltaSeconds,
	    .ManualExposure = CVarManualExposure.Get(),
	    .ExposureCompensation = CVarExposureCompensation.Get(),
	    .ExposureTargetLuminance = CVarExposureTargetLuminance.Get(),
	    .ExposureMin = CVarExposureMin.Get(),
	    .ExposureMax = CVarExposureMax.Get(),
	    .ExposureAdaptationSpeedUp = CVarExposureAdaptationSpeedUp.Get(),
	    .ExposureAdaptationSpeedDown = CVarExposureAdaptationSpeedDown.Get()};
}

ToneMappingUniformData BuildToneMappingUniformData() noexcept
{
	return ToneMappingUniformData{
	    .ToneMapper = ToneMappingValueTranslation::ToShaderToneMapper(CVarToneMapper.Get())};
}
