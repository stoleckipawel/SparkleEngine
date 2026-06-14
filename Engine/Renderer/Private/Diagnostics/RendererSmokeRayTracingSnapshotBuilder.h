#pragma once

#include "Renderer/Public/Diagnostics/RendererSmokeRayTracingDiagnostics.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

struct RayTracingPerformanceMetrics;
class RenderRayTracingScene;

namespace RendererSmokeRayTracingSnapshotBuilder
{
	RendererSmokeRayTracingDiagnostics Build(
	    const RhiRayTracingCapabilities& capabilities,
	    const RenderRayTracingScene* rayTracingScene) noexcept;
}
