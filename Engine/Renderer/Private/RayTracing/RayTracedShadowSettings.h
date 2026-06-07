#pragma once

#include "RayTracing/RayTracedShadowDenoiserMode.h"
#include "RayTracing/RayTracedShadowQualityMode.h"

#include <cstdint>

struct RayTracingCapabilityReport;

struct RayTracedShadowSettings final
{
	static constexpr std::uint32_t RaysPerPixel = 1u;

	RayTracedShadowQualityMode QualityMode = RayTracedShadowQualityMode::Hard;
	RayTracedShadowDenoiserMode DenoiserMode = RayTracedShadowDenoiserMode::NrdSigma;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	bool DiagnosticsEnabled = false;

	bool RequiresDenoiser() const noexcept
	{
		return QualityMode == RayTracedShadowQualityMode::SoftAreaLights &&
		       DenoiserMode != RayTracedShadowDenoiserMode::Off;
	}

	bool UsesHardShadowVisibility() const noexcept
	{
		return QualityMode == RayTracedShadowQualityMode::Hard;
	}

	bool UsesStochasticSoftShadowVisibility() const noexcept
	{
		return QualityMode == RayTracedShadowQualityMode::SoftAreaLights;
	}
};

RayTracedShadowSettings BuildRayTracedShadowSettingsFromCVars() noexcept;
void LogRayTracedShadowSettingsOnce(
    const RayTracedShadowSettings& settings,
    const RayTracingCapabilityReport& capabilities) noexcept;
