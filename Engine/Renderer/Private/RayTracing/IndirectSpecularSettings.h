#pragma once

#include "RayTracing/IndirectSpecularDebugMode.h"
#include "RayTracing/IndirectSpecularSampleMode.h"
#include "SceneData/MaterialBindingMode.h"

struct RayTracingCapabilityReport;

struct IndirectSpecularSettings final
{
	bool Enabled = false;
	IndirectSpecularSampleMode SampleMode = IndirectSpecularSampleMode::StochasticGGX;
	IndirectSpecularDebugMode DebugMode = IndirectSpecularDebugMode::Off;
	MaterialBindingMode MaterialMode = MaterialBindingMode::RaytracingOnly;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
};

IndirectSpecularSettings BuildIndirectSpecularSettingsFromCVars() noexcept;
void LogIndirectSpecularSettingsOnce(
    const IndirectSpecularSettings& settings,
    const RayTracingCapabilityReport& capabilities) noexcept;
