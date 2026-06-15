#pragma once

#include "../RendererAPI.h"
#include "RendererSmokeRayTracingDiagnostics.h"
#include "../../../RHI/Public/Core/RhiBackendApi.h"

#include <cstdint>
#include <string>

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
};

struct SPARKLE_RENDERER_API RendererSmokeFrameTimingDiagnostics final
{
	bool HasFinalFrameGpuMilliseconds = false;
	double FinalFrameGpuMilliseconds = 0.0;
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
