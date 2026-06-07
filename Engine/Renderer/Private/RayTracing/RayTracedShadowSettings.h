#pragma once

#include "RayTracing/RayTracedShadowDenoiserMode.h"
#include "RayTracing/RayTracedShadowQualityMode.h"

struct RayTracingCapabilityReport;

struct RayTracedShadowSettings final
{
	RayTracedShadowQualityMode QualityMode = RayTracedShadowQualityMode::SoftAreaLights;
	RayTracedShadowDenoiserMode DenoiserMode = RayTracedShadowDenoiserMode::NrdSigma;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;

	bool RequiresDenoiser() const noexcept
	{
		return QualityMode == RayTracedShadowQualityMode::SoftAreaLights &&
		       DenoiserMode != RayTracedShadowDenoiserMode::Off;
	}
};

RayTracedShadowSettings BuildRayTracedShadowSettingsFromCVars() noexcept;
void LogRayTracedShadowSettingsOnce(
    const RayTracedShadowSettings& settings,
    const RayTracingCapabilityReport& capabilities) noexcept;
