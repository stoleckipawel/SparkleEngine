#pragma once

#include "RayTracing/Diagnostics/RayTracingPerformanceMetrics.h"
#include "RayTracing/Diagnostics/RayTracingSceneDiagnostics.h"

struct RayTracingSceneDiagnosticState final
{
	RayTracingPerformanceMetrics PerformanceMetrics;
	RayTracingSceneDiagnostics SceneDiagnostics;
};

