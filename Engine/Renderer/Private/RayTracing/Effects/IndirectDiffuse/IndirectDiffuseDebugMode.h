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
	HitNormal = 7u,
	MaterialBaseColor = 8u,
	MissSkyRadiance = 9u,
	RejectionReason = 10u,
};
