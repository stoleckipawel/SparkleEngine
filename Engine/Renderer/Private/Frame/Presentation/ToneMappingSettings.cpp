#include "../../PCH.h"
#include "Frame/Presentation/ToneMappingSettings.h"

#include "Frame/Presentation/ToneMappingCVars.h"

#include <algorithm>
#include <cmath>

class ToneMappingSettingsOperations final
{
  public:
	static constexpr float kMinimumExposure = 0.000001f;
	static constexpr float kMaximumExposure = 65536.0f;
	static constexpr float kMinimumTargetLuminance = 0.0001f;
	static constexpr float kMinimumManualExposure = 0.0f;
	static constexpr float kMaximumExposureCompensation = 16.0f;
	static constexpr float kMaximumExposureAdaptationSpeed = 64.0f;

	static float SanitizeFinite(float value, float fallback) noexcept
	{
		return std::isfinite(value) ? value : fallback;
	}

	static std::uint32_t ToShaderToneMapper(EngineToneMapper toneMapper) noexcept
	{
		switch (SanitizeToneMapper(toneMapper))
		{
			case EngineToneMapper::Reinhard:
				return 0u;
			case EngineToneMapper::AcesFilmic:
				return 2u;
			case EngineToneMapper::AcesApprox:
			default:
				return 1u;
		}
	}

	static std::uint32_t ToShaderExposureMode(EngineExposureMode mode) noexcept
	{
		return SanitizeExposureMode(mode) == EngineExposureMode::Manual ? 0u : 1u;
	}

};

EngineToneMapper SanitizeToneMapper(EngineToneMapper toneMapper) noexcept
{
	switch (toneMapper)
	{
		case EngineToneMapper::Reinhard:
		case EngineToneMapper::AcesApprox:
		case EngineToneMapper::AcesFilmic:
			return toneMapper;
		default:
			return EngineToneMapper::AcesApprox;
	}
}

EngineExposureMode SanitizeExposureMode(EngineExposureMode mode) noexcept
{
	switch (mode)
	{
		case EngineExposureMode::Manual:
		case EngineExposureMode::Automatic:
			return mode;
		default:
			return EngineExposureMode::Automatic;
	}
}

EngineExposureMeteringMethod SanitizeExposureMeteringMethod(EngineExposureMeteringMethod method) noexcept
{
	switch (method)
	{
		case EngineExposureMeteringMethod::ParallelReduction:
		case EngineExposureMeteringMethod::DownsamplePyramid:
			return method;
		default:
			return EngineExposureMeteringMethod::ParallelReduction;
	}
}

float SanitizeManualExposure(float exposure) noexcept
{
	return std::clamp(ToneMappingSettingsOperations::SanitizeFinite(exposure, 1.0f), ToneMappingSettingsOperations::kMinimumManualExposure, ToneMappingSettingsOperations::kMaximumExposure);
}

float SanitizeExposureCompensation(float compensation) noexcept
{
	return std::clamp(
	    ToneMappingSettingsOperations::SanitizeFinite(compensation, 0.0f),
	    -ToneMappingSettingsOperations::kMaximumExposureCompensation,
	    ToneMappingSettingsOperations::kMaximumExposureCompensation);
}

float SanitizeExposureTargetLuminance(float luminance) noexcept
{
	return std::clamp(ToneMappingSettingsOperations::SanitizeFinite(luminance, 0.18f), ToneMappingSettingsOperations::kMinimumTargetLuminance, 1.0f);
}

float SanitizeExposureMin(float exposure) noexcept
{
	return std::clamp(ToneMappingSettingsOperations::SanitizeFinite(exposure, ToneMappingSettingsOperations::kMinimumExposure), ToneMappingSettingsOperations::kMinimumExposure, ToneMappingSettingsOperations::kMaximumExposure);
}

float SanitizeExposureMax(float exposure) noexcept
{
	return std::clamp(ToneMappingSettingsOperations::SanitizeFinite(exposure, ToneMappingSettingsOperations::kMaximumExposure), ToneMappingSettingsOperations::kMinimumExposure, ToneMappingSettingsOperations::kMaximumExposure);
}

float SanitizeExposureAdaptationSpeed(float speed) noexcept
{
	return std::clamp(ToneMappingSettingsOperations::SanitizeFinite(speed, 0.0f), 0.0f, ToneMappingSettingsOperations::kMaximumExposureAdaptationSpeed);
}

void SanitizeExposureRange(float& minExposure, float& maxExposure) noexcept
{
	minExposure = SanitizeExposureMin(minExposure);
	maxExposure = SanitizeExposureMax(maxExposure);
	if (minExposure > maxExposure)
	{
		std::swap(minExposure, maxExposure);
	}
}

ExposureUniformData BuildExposureUniformData(float frameDeltaSeconds, bool exposureHistoryValid) noexcept
{
	float minExposure = CVarExposureMin.Get();
	float maxExposure = CVarExposureMax.Get();
	SanitizeExposureRange(minExposure, maxExposure);

	return ExposureUniformData{
	    .ExposureMode = ToneMappingSettingsOperations::ToShaderExposureMode(CVarExposureMode.Get()),
	    .ExposureHistoryValid = exposureHistoryValid ? 1u : 0u,
	    .FrameDeltaSeconds = std::clamp(ToneMappingSettingsOperations::SanitizeFinite(frameDeltaSeconds, 1.0f / 60.0f), 0.0f, 1.0f),
	    .ManualExposure = SanitizeManualExposure(CVarManualExposure.Get()),
	    .ExposureCompensation = SanitizeExposureCompensation(CVarExposureCompensation.Get()),
	    .ExposureTargetLuminance = SanitizeExposureTargetLuminance(CVarExposureTargetLuminance.Get()),
	    .ExposureMin = minExposure,
	    .ExposureMax = maxExposure,
	    .ExposureAdaptationSpeedUp = SanitizeExposureAdaptationSpeed(CVarExposureAdaptationSpeedUp.Get()),
	    .ExposureAdaptationSpeedDown = SanitizeExposureAdaptationSpeed(CVarExposureAdaptationSpeedDown.Get())};
}

ToneMappingUniformData BuildToneMappingUniformData() noexcept
{
	return ToneMappingUniformData{
	    .ToneMapper = ToneMappingSettingsOperations::ToShaderToneMapper(CVarToneMapper.Get())};
}
