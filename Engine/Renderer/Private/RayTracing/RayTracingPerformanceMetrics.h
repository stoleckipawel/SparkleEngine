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
	ERhiPartitionedTlasProvider PartitionedTlasProvider = ERhiPartitionedTlasProvider::None;
	bool SupportsPartitionedTlas = false;
};

struct RayTracingFrameTimingMetrics final
{
	double ScenePrepareCpuMilliseconds = 0.0;
	double SceneBuildCpuMilliseconds = 0.0;
	double RayTracingPassGpuMilliseconds = 0.0;
};

struct RayTracingPerformanceMetrics final
{
	RayTracingProviderMetrics Providers;
	RayTracingFrameTimingMetrics Timings;
	RayTracingBlasMetrics Blas;
	RayTracingClassicTlasMetrics ClassicTlas;
	RayTracingPtlasPlannerMetrics PtlasPlanner;
	RayTracingPtlasGpuUpdateMetrics PtlasGpuUpdates;
};
