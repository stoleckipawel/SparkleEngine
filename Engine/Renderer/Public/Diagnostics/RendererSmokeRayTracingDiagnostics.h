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
	double CpuPackMilliseconds = 0.0;
	double GpuDirtyDetectionMilliseconds = 0.0;
	double GpuNativePackMilliseconds = 0.0;
	double PtlasUpdateGpuMilliseconds = 0.0;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingCapabilityDiagnostics final
{
	bool Supported = false;
	bool InlineRayQuerySupported = false;
	ERhiRayTracingTopLevelProvider TopLevelProvider = ERhiRayTracingTopLevelProvider::None;
	std::string TopLevelProviderReason;
	std::string PartitionedTlasCapabilityReason;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingFrameTimingDiagnostics final
{
	double ScenePrepareCpuMilliseconds = 0.0;
	double SceneBuildCpuMilliseconds = 0.0;
	double RayTracingPassGpuMilliseconds = 0.0;
	double IndirectSpecularGpuMilliseconds = 0.0;
};

struct SPARKLE_RENDERER_API RendererSmokeIndirectSpecularDiagnostics final
{
	std::string StatusReason;
	bool Enabled = false;
	std::uint32_t SampleMode = 0;
	std::uint32_t DebugMode = 0;
	float MaxDistance = 0.0f;
	bool HitDataAvailable = false;
	std::uint32_t HitInstanceCount = 0;
	std::uint32_t HitMaterialCount = 0;
	std::string GpuTimingLabel;
};

struct SPARKLE_RENDERER_API RendererSmokeRayTracingDiagnostics final
{
	RendererSmokeRayTracingCapabilityDiagnostics Capability;
	RendererSmokeRayTracingFrameTimingDiagnostics FrameTimings;
	RendererSmokeIndirectSpecularDiagnostics IndirectSpecular;
	RendererSmokeRayTracingBlasDiagnostics Blas;
	RendererSmokeRayTracingClassicTlasDiagnostics ClassicTlas;
	RendererSmokeRayTracingPtlasPlannerDiagnostics PtlasPlanner;
	RendererSmokeRayTracingPtlasGpuUpdateDiagnostics PtlasGpuUpdates;
};
