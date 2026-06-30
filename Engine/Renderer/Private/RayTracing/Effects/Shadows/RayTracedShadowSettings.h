#pragma once

#include "RayTracing/Effects/Shadows/RayTracedShadowDenoiserMode.h"

#include <cstdint>

struct RayTracingCapabilityReport;

struct RayTracedShadowSettings final
{
	static constexpr std::uint32_t RaysPerPixel = 1u;

	RayTracedShadowDenoiserMode DenoiserMode = RayTracedShadowDenoiserMode::NrdSigma;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	bool Enabled = true;
	bool DiagnosticsEnabled = false;
};

RayTracedShadowSettings BuildRayTracedShadowSettingsFromCVars() noexcept;
void LogRayTracedShadowSettingsOnce(
    const RayTracedShadowSettings& settings,
    const RayTracingCapabilityReport& capabilities) noexcept;
