#pragma once

#include "../RendererAPI.h"
#include "../../../RHI/Public/Core/RhiBackendApi.h"
#include "../../../RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <string>

struct SPARKLE_RENDERER_API RendererSmokeDiagnosticsSnapshot final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	std::uint32_t FrameGraphUnresolvedBarrierWarnings = 0;
	std::string UpscalerProvider;
	std::string UpscalerStatus;
	std::string UpscalerReason;
	bool RayTracingSupported = false;
	bool InlineRayQuerySupported = false;
	bool RayTracingTlasValid = false;
	std::uint32_t RayTracingTlasInstanceCount = 0;
	ERhiRayTracingTopLevelProvider RayTracingTopLevelProvider = ERhiRayTracingTopLevelProvider::None;
	ERhiPartitionedTlasProvider RayTracingPartitionedTlasProvider = ERhiPartitionedTlasProvider::None;
	bool RayTracingPartitionedTlasSupported = false;
	std::uint32_t RayTracingReferencedMeshCount = 0;
	std::uint32_t RayTracingBuiltBlasCount = 0;
	std::uint32_t RayTracingReusedBlasCount = 0;
	std::uint32_t RayTracingCandidateInstanceCount = 0;
	std::uint32_t RayTracingMissingGpuMeshCount = 0;
	std::uint32_t RayTracingRejectedBlasCount = 0;
	bool RayTracingBuiltTlas = false;
	double RayTracingScenePrepareCpuMilliseconds = 0.0;
	double RayTracingSceneBuildCpuMilliseconds = 0.0;
	double RayTracingBlasCpuMilliseconds = 0.0;
	double RayTracingTlasCpuMilliseconds = 0.0;
	double RayTracingTlasInstancePreparationCpuMilliseconds = 0.0;
	double RayTracingBlasGpuMilliseconds = 0.0;
	double RayTracingClassicTlasGpuMilliseconds = 0.0;
	double RayTracingPassGpuMilliseconds = 0.0;
};
