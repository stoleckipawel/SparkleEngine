#pragma once

#include "RayTracing/RTIndirectSpecularDebugMode.h"
#include "RayTracing/RTIndirectSpecularSampleMode.h"

struct RayTracingCapabilityReport;

struct RTIndirectSpecularSettings final
{
	bool Enabled = false;
	RTIndirectSpecularSampleMode SampleMode = RTIndirectSpecularSampleMode::StochasticGGX;
	RTIndirectSpecularDebugMode DebugMode = RTIndirectSpecularDebugMode::Off;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
};

RTIndirectSpecularSettings BuildRTIndirectSpecularSettingsFromCVars() noexcept;
void LogRTIndirectSpecularSettingsOnce(
    const RTIndirectSpecularSettings& settings,
    const RayTracingCapabilityReport& capabilities) noexcept;
