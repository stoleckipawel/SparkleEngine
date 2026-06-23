#pragma once

#include "../RendererAPI.h"
#include "../../../RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <cstdint>
#include <string>

struct SPARKLE_RENDERER_API RendererSmokeRayTracingBlasDiagnostics final
{
	std::uint32_t ReferencedMeshCount = 0;
	std::uint32_t BuiltCount = 0;
	std::uint32_t ReusedCount = 0;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingClassicTlasDiagnostics final
{
	bool Valid = false;
	std::uint32_t InstanceCount = 0;
	std::uint32_t CandidateInstanceCount = 0;
	std::uint32_t MissingGpuMeshCount = 0;
	std::uint32_t RejectedBlasCount = 0;
	bool Built = false;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingPtlasPlannerDiagnostics final
{
	ERhiPartitionedTlasProvider Provider = ERhiPartitionedTlasProvider::None;
	bool Supported = false;
	std::uint32_t TotalRenderInstanceCount = 0;
	std::uint32_t TraceableInstanceCount = 0;
	std::uint32_t StaticTraceableInstanceCount = 0;
	std::uint32_t DynamicTraceableInstanceCount = 0;
	std::uint32_t PartitionsPerAxis = 0;
	std::uint32_t PartitionCount = 0;
	std::uint32_t GridPartitionCount = 0;
	std::uint32_t DirtyTransformCount = 0;
	std::uint32_t MovedPartitionCount = 0;
	std::uint32_t GlobalPartitionEligibleCount = 0;
	std::uint32_t GlobalPartitionInstanceCount = 0;
	std::uint32_t ActivePartitionCount = 0;
	std::uint32_t MaxPartitionActivityCount = 0;
	std::uint32_t DuplicateStableIndexCount = 0;
	bool Overflow = false;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingPtlasGpuUpdateDiagnostics final
{
	ERhiPartitionedTlasOperationWriterPath RequestedWriterPath = ERhiPartitionedTlasOperationWriterPath::CpuPack;
	ERhiPartitionedTlasOperationWriterPath SelectedWriterPath = ERhiPartitionedTlasOperationWriterPath::CpuPack;
	const char* WriterSelectionReason = "ptlas-operation-writer-cpu-pack-selected";
	std::uint32_t LogicalUpdateCount = 0;
	std::uint32_t NativeOperationCount = 0;
	std::uint32_t ValidationMismatchCount = 0;
	bool GpuDrivenOperationApiSupported = false;
	bool GpuLogicalUpdateWriterAvailable = false;
	bool FullGpuNativePackAvailable = false;
	bool FullGpuNativePackSubmitted = false;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingCapabilityDiagnostics final
{
	bool Supported = false;
	bool InlineRayQuerySupported = false;
	ERhiRayTracingTopLevelProvider TopLevelProvider = ERhiRayTracingTopLevelProvider::None;
	std::string TopLevelProviderReason;
	std::string PartitionedTlasCapabilityReason;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingDiagnostics final
{
	RendererSmokeRayTracingCapabilityDiagnostics Capability;
	RendererSmokeRayTracingBlasDiagnostics Blas;
	RendererSmokeRayTracingClassicTlasDiagnostics ClassicTlas;
	RendererSmokeRayTracingPtlasPlannerDiagnostics PtlasPlanner;
	RendererSmokeRayTracingPtlasGpuUpdateDiagnostics PtlasGpuUpdates;
};
