#pragma once

#include "../RendererAPI.h"
#include "RendererDiagnosticsSnapshot.h"
#include "RendererSmokeRayTracingDiagnostics.h"
#include "../../../RHI/Public/Core/RhiBackendApi.h"

#include <cstdint>
#include <string>
#include <vector>

struct SPARKLE_RENDERER_API RendererSmokeAdapterDiagnostics final
{
	std::string Name;
	std::string DriverDescription;
	std::uint32_t VendorId = 0;
	std::uint32_t DeviceId = 0;
};

struct SPARKLE_RENDERER_API RendererSmokeFrameGraphDiagnostics final
{
	std::uint32_t UnresolvedBarrierWarnings = 0;
	std::uint32_t MissingExecutionBindings = 0;
	std::uint32_t TransientResources = 0;
	std::uint32_t ImportedResources = 0;
	std::uint32_t PersistentResources = 0;
	std::uint32_t ViewportProducts = 0;
};

struct SPARKLE_RENDERER_API RendererSmokeFrameTimingDiagnostics final
{
	bool HasFinalFrameGpuMilliseconds = false;
	double FinalFrameGpuMilliseconds = 0.0;
	std::vector<RendererGpuTimingMetric> GpuTimings;
};

struct SPARKLE_RENDERER_API RendererSmokeUpscalerDiagnostics final
{
	std::string Provider;
	std::string Status;
	std::string Reason;
};

struct SPARKLE_RENDERER_API RendererSmokeDiagnosticsSnapshot final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	RendererSmokeAdapterDiagnostics Adapter;
	RendererSmokeFrameGraphDiagnostics FrameGraph;
	RendererSmokeFrameTimingDiagnostics FrameTimings;
	RendererSmokeUpscalerDiagnostics Upscaler;
	RendererSmokeRayTracingDiagnostics RayTracing;
};
