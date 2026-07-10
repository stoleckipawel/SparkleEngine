#include "PCH.h"

#include "Renderer/Public/Debug/RendererCVars.h"

ConsoleVariable<RenderViewMode> CVarRenderViewMode("r.ViewMode", RenderViewMode::Lit, "Renderer debug view mode.");
ConsoleVariable<GBufferMode> CVarGBufferMode(
    "r.GBuffer.Mode",
    GBufferMode::Rasterized,
    "GBuffer producer mode: 0=rasterized, 1=raytraced.");
ConsoleVariable<LightingMode> CVarLightingMode(
    "r.Lighting.Mode",
    LightingMode::RestirPathTraced,
    "Lighting pipeline: 0=ReSTIR real-time path tracing, 1=convergent reference path tracing.");

LightingMode GetLightingMode() noexcept
{
	switch (CVarLightingMode.Get())
	{
		case LightingMode::ReferencePathTraced:
			return LightingMode::ReferencePathTraced;
		case LightingMode::RestirPathTraced:
		default:
			return LightingMode::RestirPathTraced;
	}
}

ConsoleVariable<bool> CVarRendererMeshAutoBatching(
    "r.MeshAutoBatching",
    true,
    "Build renderer-side auto batches for compatible flat mesh instances.");
ConsoleVariable<RendererDiagnosticMarkerVerbosity> CVarRendererDiagnosticMarkerVerbosity(
    "r.Diagnostics.MarkerVerbosity",
    RendererDiagnosticMarkerVerbosity::FramePass,
    "Renderer diagnostic marker verbosity: 0=off, 1=frame and frame-graph passes, 2=detailed markers.");
ConsoleVariable<bool> CVarRendererDiagnosticGpuTiming(
    "r.Diagnostics.GpuTiming",
    false,
    "Collect internal GPU timestamp timings from renderer diagnostic scopes. Profiler markers remain controlled by "
    "r.Diagnostics.MarkerVerbosity.");
ConsoleVariable<bool> CVarRayTracingClassicTlasRefit(
    "r.RayTracing.Tlas.Refit",
    true,
    "Use classic TLAS refit/update after the initial full build when PTLAS is disabled.");
ConsoleVariable<std::uint32_t> CVarRayTracingPartitionsPerAxis(
    "r.RayTracing.Ptlas.PartitionsPerAxis",
    8u,
    "Logical ray tracing partition grid resolution per axis.");
ConsoleVariable<RayTracingPtlasPartitionUpdateMode> CVarRayTracingPtlasPartitionUpdateMode(
    "r.RayTracing.Ptlas.PartitionUpdateMode",
    RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition,
    "PTLAS partition update mode: 0=always update partition, 1=always move dynamic to global, 2=update nearby and move distant dynamic to "
    "global.");
ConsoleVariable<bool> CVarRayTracingPtlasMarkAllDynamicInPartition(
    "r.RayTracing.Ptlas.MarkAllDynamicInPartition",
    false,
    "Marks all dynamic instances in a touched partition as dynamic, matching the NVIDIA PTLAS demo control.");
ConsoleVariable<float> CVarRayTracingPtlasModeChangeDistance(
    "r.RayTracing.Ptlas.ModeChangeDistance",
    100.0f,
    "Distance threshold for switching dynamic instances between nearby partition updates and the global partition.");
