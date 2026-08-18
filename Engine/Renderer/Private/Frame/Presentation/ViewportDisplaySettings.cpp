#include "../../PCH.h"

#include "Frame/Presentation/ViewportDisplaySettings.h"

#include "Frame/Presentation/ToneMappingCVars.h"

#include <algorithm>

EngineExposureMode ResolvedViewportDisplaySettings::ResolveMode(EngineExposureMode requested, EngineExposureMode fallback) noexcept
{
	switch (requested)
	{
		case EngineExposureMode::Manual:
		case EngineExposureMode::Automatic:
			return requested;
		default:
			return fallback;
	}
}

EngineExposureMeteringMethod ResolvedViewportDisplaySettings::ResolveMeteringMethod(
    EngineExposureMeteringMethod requested,
    EngineExposureMeteringMethod fallback) noexcept
{
	switch (requested)
	{
		case EngineExposureMeteringMethod::ParallelReduction:
		case EngineExposureMeteringMethod::DownsamplePyramid:
			return requested;
		default:
			return fallback;
	}
}

ResolvedViewportDisplaySettings ResolvedViewportDisplaySettings::Resolve(const ViewportExposureOverrides& overrides) noexcept
{
	ResolvedViewportDisplaySettings resolved{
	    .ToneMapper = CVarToneMapper.Get(),
	    .ExposureMode = CVarExposureMode.Get(),
	    .ExposureMeteringMethod = CVarExposureMeteringMethod.Get(),
	    .ManualExposure = CVarManualExposure.Get(),
	    .ExposureCompensation = CVarExposureCompensation.Get(),
	    .ExposureTargetLuminance = CVarExposureTargetLuminance.Get(),
	    .ExposureMin = CVarExposureMin.Get(),
	    .ExposureMax = CVarExposureMax.Get(),
	    .ExposureAdaptationSpeedUp = CVarExposureAdaptationSpeedUp.Get(),
	    .ExposureAdaptationSpeedDown = CVarExposureAdaptationSpeedDown.Get()};
	if (overrides.OverrideMode)
	{
		resolved.ExposureMode = ResolveMode(overrides.Mode, resolved.ExposureMode);
	}
	if (overrides.OverrideMeteringMethod)
	{
		resolved.ExposureMeteringMethod = ResolveMeteringMethod(overrides.MeteringMethod, resolved.ExposureMeteringMethod);
	}
	if (overrides.OverrideManualExposure)
	{
		resolved.ManualExposure = overrides.ManualExposure;
	}
	if (overrides.OverrideCompensation)
	{
		resolved.ExposureCompensation = overrides.Compensation;
	}
	if (overrides.OverrideTargetLuminance)
	{
		resolved.ExposureTargetLuminance = overrides.TargetLuminance;
	}
	if (overrides.OverrideMinimum)
	{
		resolved.ExposureMin = overrides.Minimum;
	}
	if (overrides.OverrideMaximum)
	{
		resolved.ExposureMax = overrides.Maximum;
	}
	if (overrides.OverrideAdaptationSpeedUp)
	{
		resolved.ExposureAdaptationSpeedUp = overrides.AdaptationSpeedUp;
	}
	if (overrides.OverrideAdaptationSpeedDown)
	{
		resolved.ExposureAdaptationSpeedDown = overrides.AdaptationSpeedDown;
	}

	resolved.ManualExposure = (std::max) (resolved.ManualExposure, 0.0f);
	resolved.ExposureCompensation = std::clamp(resolved.ExposureCompensation, -16.0f, 16.0f);
	resolved.ExposureTargetLuminance = (std::max) (resolved.ExposureTargetLuminance, 0.0001f);
	resolved.ExposureMin = (std::max) (resolved.ExposureMin, 0.0f);
	resolved.ExposureMax = (std::max) (resolved.ExposureMax, resolved.ExposureMin);
	resolved.ExposureAdaptationSpeedUp = (std::max) (resolved.ExposureAdaptationSpeedUp, 0.0f);
	resolved.ExposureAdaptationSpeedDown = (std::max) (resolved.ExposureAdaptationSpeedDown, 0.0f);
	return resolved;
}
