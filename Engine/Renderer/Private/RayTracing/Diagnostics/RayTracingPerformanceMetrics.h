#pragma once

#include "RayTracing/Diagnostics/RayTracingBlasMetrics.h"
#include "RayTracing/Diagnostics/RayTracingClassicTlasMetrics.h"
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
};

