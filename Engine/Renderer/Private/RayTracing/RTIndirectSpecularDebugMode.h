#pragma once

#include "RayTracing/RayTracingDebugMode.h"

#include <cstdint>

enum class RTIndirectSpecularDebugMode : std::uint32_t
{
	Off = RayTracingDebugMode::Off,
	HitMask = RayTracingDebugMode::HitMask,
	HitDistance = RayTracingDebugMode::HitDistance,
	MirrorDirection = 3u,
	HitUV = RayTracingDebugMode::HitUV,
	HitNormal = RayTracingDebugMode::HitNormal,
	MaterialId = RayTracingDebugMode::MaterialId,
	GeometryClass = RayTracingDebugMode::GeometryClass,
	HitRejectionReason = RayTracingDebugMode::HitRejectionReason,
	SampleDirection = 10u,
	SamplePdf = 11u,
	SampleThroughput = 12u,
	HitRadiance = 13u,
	FinalContribution = 14u,
	MaterialBaseColor = RayTracingDebugMode::MaterialBaseColor,
	MaterialRoughnessMetallic = RayTracingDebugMode::MaterialRoughnessMetallic,
	MaterialEmissive = RayTracingDebugMode::MaterialEmissive,
	HitTangent = RayTracingDebugMode::HitTangent,
	HitBitangent = RayTracingDebugMode::HitBitangent,
	HitNormalTangent = RayTracingDebugMode::HitNormalTangent,
	HitSampledNormal = RayTracingDebugMode::HitSampledNormal,
	AlphaAcceptedRejected = RayTracingDebugMode::AlphaAcceptedRejected,
	AlphaSample = RayTracingDebugMode::AlphaSample,
	AlphaCutoff = RayTracingDebugMode::AlphaCutoff,
};
