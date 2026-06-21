#pragma once

#include "RayTracing/RayTracingBlasMetrics.h"
#include "RayTracing/RayTracingClassicTlasMetrics.h"
#include "RayTracing/RayTracingPtlasGpuUpdateMetrics.h"
#include "RayTracing/RayTracingPtlasPlannerMetrics.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>

struct RayTracingProviderMetrics final
{
	ERhiRayTracingTopLevelProvider TopLevelProvider = ERhiRayTracingTopLevelProvider::None;
	const char* TopLevelProviderReason = "not-queried";
	ERhiPartitionedTlasProvider PartitionedTlasProvider = ERhiPartitionedTlasProvider::None;
	bool SupportsPartitionedTlas = false;
	const char* PartitionedTlasCapabilityReason = "not-queried";
};

struct RayTracingPerformanceMetrics final
{
	RayTracingProviderMetrics Providers;
	RayTracingBlasMetrics Blas;
	RayTracingClassicTlasMetrics ClassicTlas;
	RayTracingPtlasPlannerMetrics PtlasPlanner;
	RayTracingPtlasGpuUpdateMetrics PtlasGpuUpdates;
};
