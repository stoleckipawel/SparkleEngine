#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "Renderer/Public/RendererAPI.h"

#include <cstdint>

enum class RendererDiagnosticMarkerVerbosity : std::uint8_t
{
	Off = 0,
	FramePass = 1,
	Detailed = 2,
};

extern SPARKLE_RENDERER_API ConsoleVariable<RenderViewMode> CVarRenderViewMode;
extern SPARKLE_RENDERER_API ConsoleVariable<GBufferAlgorithm> CVarGBufferAlgorithm;
extern SPARKLE_RENDERER_API ConsoleVariable<RayTracingExecutionMode> CVarGBufferRayTracingExecutionMode;
extern SPARKLE_RENDERER_API ConsoleVariable<LightingMode> CVarLightingMode;
extern SPARKLE_RENDERER_API ConsoleVariable<bool> CVarRendererMeshAutoBatching;
extern SPARKLE_RENDERER_API ConsoleVariable<RendererDiagnosticMarkerVerbosity> CVarRendererDiagnosticMarkerVerbosity;
extern SPARKLE_RENDERER_API ConsoleVariable<bool> CVarRendererDiagnosticGpuTiming;
extern SPARKLE_RENDERER_API ConsoleVariable<bool> CVarRendererParallelFrameGraphRecording;
extern SPARKLE_RENDERER_API ConsoleVariable<bool> CVarRayTracingClassicTlasRefit;
extern SPARKLE_RENDERER_API ConsoleVariable<std::uint32_t> CVarRayTracingPartitionsPerAxis;
extern SPARKLE_RENDERER_API ConsoleVariable<RayTracingPtlasPartitionUpdateMode> CVarRayTracingPtlasPartitionUpdateMode;
extern SPARKLE_RENDERER_API ConsoleVariable<bool> CVarRayTracingPtlasMarkAllDynamicInPartition;
extern SPARKLE_RENDERER_API ConsoleVariable<float> CVarRayTracingPtlasModeChangeDistance;
