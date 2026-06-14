#pragma once

#include "../RendererAPI.h"
#include "../../../RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>

struct SPARKLE_RENDERER_API RendererSmokeRayTracingBlasDiagnostics final
{
	std::uint32_t ReferencedMeshCount = 0;
	std::uint32_t BuiltCount = 0;
	std::uint32_t ReusedCount = 0;
	double CpuMilliseconds = 0.0;
	double GpuMilliseconds = 0.0;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingClassicTlasDiagnostics final
{
	bool Valid = false;
	std::uint32_t InstanceCount = 0;
	std::uint32_t CandidateInstanceCount = 0;
	std::uint32_t MissingGpuMeshCount = 0;
	std::uint32_t RejectedBlasCount = 0;
	bool Built = false;
	double CpuMilliseconds = 0.0;
	double InstancePreparationCpuMilliseconds = 0.0;
	double GpuMilliseconds = 0.0;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingPtlasPlannerDiagnostics final
{
	ERhiPartitionedTlasProvider Provider = ERhiPartitionedTlasProvider::None;
	bool Supported = false;
	std::uint32_t PartitionCount = 0;
	std::uint32_t DirtyTransformCount = 0;
	std::uint32_t MovedPartitionCount = 0;
	std::uint32_t GlobalPartitionInstanceCount = 0;
	std::uint32_t DuplicateStableIndexCount = 0;
	bool Overflow = false;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingCapabilityDiagnostics final
{
	bool Supported = false;
	bool InlineRayQuerySupported = false;
	ERhiRayTracingTopLevelProvider TopLevelProvider = ERhiRayTracingTopLevelProvider::None;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingFrameTimingDiagnostics final
{
	double ScenePrepareCpuMilliseconds = 0.0;
	double SceneBuildCpuMilliseconds = 0.0;
	double RayTracingPassGpuMilliseconds = 0.0;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingDiagnostics final
{
	RendererSmokeRayTracingCapabilityDiagnostics Capability;
	RendererSmokeRayTracingFrameTimingDiagnostics FrameTimings;
	RendererSmokeRayTracingBlasDiagnostics Blas;
	RendererSmokeRayTracingClassicTlasDiagnostics ClassicTlas;
	RendererSmokeRayTracingPtlasPlannerDiagnostics PtlasPlanner;
};
