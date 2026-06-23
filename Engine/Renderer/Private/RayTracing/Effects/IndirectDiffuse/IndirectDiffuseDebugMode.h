#pragma once

#include "RayTracing/RayTracingDebugMode.h"

#include <cstdint>

enum class IndirectDiffuseDebugMode : std::uint32_t
{
	Off = RayTracingDebugMode::Off,
	HitMask = RayTracingDebugMode::HitMask,
	HitDistance = RayTracingDebugMode::HitDistance,
	SampleDirection = 3u,
	SamplePdf = 4u,
	HitRadiance = 5u,
	FinalContribution = 6u,
};
