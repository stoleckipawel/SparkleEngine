#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"

#include <cstdint>

enum class RendererDiagnosticMarkerVerbosity : std::uint8_t
{
	Off = 0,
	FramePass = 1,
	Detailed = 2,
};

extern ConsoleVariable<GBufferAlgorithm> CVarGBufferAlgorithm;
extern ConsoleVariable<bool> CVarRendererMeshAutoBatching;
extern ConsoleVariable<RendererDiagnosticMarkerVerbosity> CVarRendererDiagnosticMarkerVerbosity;
extern ConsoleVariable<bool> CVarRendererDiagnosticGpuTiming;
extern ConsoleVariable<bool> CVarRendererParallelFrameGraphRecording;
extern ConsoleVariable<bool> CVarRayTracingClassicTlasRefit;
extern ConsoleVariable<std::uint32_t> CVarRayTracingPartitionsPerAxis;
extern ConsoleVariable<RayTracingPtlasPartitionUpdateMode> CVarRayTracingPtlasPartitionUpdateMode;
extern ConsoleVariable<bool> CVarRayTracingPtlasMarkAllDynamicInPartition;
extern ConsoleVariable<float> CVarRayTracingPtlasModeChangeDistance;
