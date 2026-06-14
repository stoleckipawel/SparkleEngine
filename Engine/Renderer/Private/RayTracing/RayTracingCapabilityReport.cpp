#include "../PCH.h"
#include "RayTracing/RayTracingCapabilityReport.h"

#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Core/RhiBackendSelection.h"

namespace
{
	constexpr const char* BoolToString(bool value) noexcept
	{
		return value ? "true" : "false";
	}
}  // namespace

bool RayTracingCapabilityReport::CanUseInlineRayQueryShadows() const noexcept
{
	return Core.SupportsRayTracing && Core.SupportsInlineRayQuery && AccelerationStructures.HasAccelerationStructureAlignment &&
	       AccelerationStructures.HasScratchBufferAlignment && AccelerationStructures.HasInstanceDescSize;
}

const char* RayTracingCapabilityReport::GetInlineRayQueryShadowUnavailableReason() const noexcept
{
	if (!Core.SupportsRayTracing)
	{
		return "ray-tracing-unsupported";
	}
	if (!Core.SupportsInlineRayQuery)
	{
		return "inline-ray-query-unsupported";
	}
	if (!AccelerationStructures.HasAccelerationStructureAlignment)
	{
		return "missing-acceleration-structure-alignment";
	}
	if (!AccelerationStructures.HasScratchBufferAlignment)
	{
		return "missing-scratch-buffer-alignment";
	}
	if (!AccelerationStructures.HasInstanceDescSize)
	{
		return "missing-instance-desc-size";
	}
	return "available";
}

RayTracingCapabilityReport RayTracingCapabilityReporter::Build(const RhiCapabilities& capabilities) noexcept
{
	return Build(capabilities.BackendApi, capabilities.RayTracing);
}

void RayTracingCapabilityReporter::LogOnce(const RayTracingCapabilityReport& report) noexcept
{
	static bool s_logged = false;
	if (s_logged)
	{
		return;
	}

	s_logged = true;
	const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.RayTracing");
	SPDLOG_LOGGER_INFO(
	    logger,
	    "Ray tracing capability summary: backend={} rayTracing={} inlineRayQuery={} asAlignment={} scratchAlignment={} "
	    "instanceDescSize={} maxRecursionDepth={} maxPayloadBytes={} maxAttributeBytes={} inlineShadowReady={} inlineShadowReason={} "
	    "topLevelProvider={}({}) partitionedTlasProvider={} supported={} requiresNvidia={} nvidiaDevice={} "
	    "vulkanNv={} vulkanExtension={} vulkanFeature={} vulkanFunctions={} vulkanDescriptors={} "
	    "d3d12Nvapi={} d3d12NvapiHeaders={} d3d12NvapiRuntime={} d3d12DeviceInterface={} d3d12CommandListInterface={} "
	    "d3d12PublicDxr={} d3d12PublicDxrHeaders={} cpuPackOps={} gpuDrivenOps={} partitionedReason={}",
	    RhiBackendApiToString(report.BackendApi),
	    BoolToString(report.Core.SupportsRayTracing),
	    BoolToString(report.Core.SupportsInlineRayQuery),
	    report.AccelerationStructures.AccelerationStructureByteAlignment,
	    report.AccelerationStructures.ScratchBufferByteAlignment,
	    report.AccelerationStructures.InstanceDescSizeInBytes,
	    report.Core.MaxTraceRecursionDepth,
	    report.Core.MaxRayPayloadSizeInBytes,
	    report.Core.MaxRayAttributeSizeInBytes,
	    BoolToString(report.CanUseInlineRayQueryShadows()),
	    report.GetInlineRayQueryShadowUnavailableReason(),
	    RhiRayTracingTopLevelProviderToString(report.TopLevelProvider.SelectedProvider),
	    report.TopLevelProvider.SelectionReason,
	    RhiPartitionedTlasProviderToString(report.PartitionedTlas.Provider),
	    BoolToString(report.PartitionedTlas.Supported),
	    BoolToString(report.PartitionedTlas.RequiresNvidiaDevice),
	    BoolToString(report.PartitionedTlas.RunsOnNvidiaDevice),
	    BoolToString(report.PartitionedTlas.SupportsVulkanNativeProvider),
	    BoolToString(report.PartitionedTlas.SupportsVulkanExtension),
	    BoolToString(report.PartitionedTlas.SupportsVulkanFeatureQuery),
	    BoolToString(report.PartitionedTlas.SupportsVulkanFunctionLoading),
	    BoolToString(report.PartitionedTlas.SupportsVulkanDescriptorPath),
	    BoolToString(report.PartitionedTlas.SupportsD3D12NvapiProvider),
	    BoolToString(report.PartitionedTlas.SupportsD3D12NvapiHeaders),
	    BoolToString(report.PartitionedTlas.SupportsD3D12NvapiRuntime),
	    BoolToString(report.PartitionedTlas.SupportsD3D12DeviceInterface),
	    BoolToString(report.PartitionedTlas.SupportsD3D12CommandListInterface),
	    BoolToString(report.PartitionedTlas.SupportsD3D12PublicDxrProvider),
	    BoolToString(report.PartitionedTlas.SupportsD3D12PublicDxrHeaders),
	    BoolToString(report.PartitionedTlas.SupportsCpuPackedOperations),
	    BoolToString(report.PartitionedTlas.SupportsGpuDrivenOperations),
	    report.PartitionedTlas.CapabilityStatusReason);
}

RayTracingCapabilityReport RayTracingCapabilityReporter::Build(
    ERhiBackendApi backendApi,
    const RhiRayTracingCapabilities& rayTracing) noexcept
{
	return RayTracingCapabilityReport{
	    .BackendApi = backendApi,
	    .Core =
	        RayTracingCoreCapabilityReport{
	            .SupportsRayTracing = rayTracing.SupportsRayTracing,
	            .SupportsInlineRayQuery = rayTracing.SupportsInlineRayQuery,
	            .MaxTraceRecursionDepth = rayTracing.MaxTraceRecursionDepth,
	            .MaxRayPayloadSizeInBytes = rayTracing.MaxRayPayloadSizeInBytes,
	            .MaxRayAttributeSizeInBytes = rayTracing.MaxRayAttributeSizeInBytes},
	    .AccelerationStructures =
	        RayTracingAccelerationStructureCapabilityReport{
	            .HasAccelerationStructureAlignment = rayTracing.AccelerationStructureByteAlignment != 0,
	            .HasScratchBufferAlignment = rayTracing.ScratchBufferByteAlignment != 0,
	            .HasInstanceDescSize = rayTracing.InstanceDescSizeInBytes != 0,
	            .AccelerationStructureByteAlignment = rayTracing.AccelerationStructureByteAlignment,
	            .ScratchBufferByteAlignment = rayTracing.ScratchBufferByteAlignment,
	            .InstanceDescSizeInBytes = rayTracing.InstanceDescSizeInBytes},
	    .TopLevelProvider =
	        RayTracingTopLevelProviderCapabilityReport{
	            .SelectedProvider = rayTracing.Groups.Provider.SelectedTopLevelProvider,
	            .SelectionReason = rayTracing.Groups.Provider.SelectedTopLevelProviderReason},
	    .PartitionedTlas =
	        RayTracingPartitionedTlasCapabilityReport{
	            .Provider = rayTracing.Groups.PartitionedTlas.Provider,
	            .Supported = rayTracing.Groups.PartitionedTlas.Supported,
	            .RequiresNvidiaDevice = rayTracing.Groups.PartitionedTlas.RequiresNvidiaDevice,
	            .RunsOnNvidiaDevice = rayTracing.Groups.PartitionedTlas.RunsOnNvidiaDevice,
	            .SupportsVulkanNativeProvider =
	                rayTracing.Groups.PartitionedTlas.SupportsVulkanNativePartitionedAccelerationStructure,
	            .SupportsVulkanExtension = rayTracing.Groups.PartitionedTlas.SupportsVulkanExtension,
	            .SupportsVulkanFeatureQuery = rayTracing.Groups.PartitionedTlas.SupportsVulkanFeatureQuery,
	            .SupportsVulkanFunctionLoading = rayTracing.Groups.PartitionedTlas.SupportsVulkanFunctionLoading,
	            .SupportsVulkanDescriptorPath = rayTracing.Groups.PartitionedTlas.SupportsVulkanDescriptorPath,
	            .SupportsD3D12NvapiProvider = rayTracing.Groups.PartitionedTlas.SupportsD3D12NvapiPartitionedTlas,
	            .SupportsD3D12NvapiHeaders = rayTracing.Groups.PartitionedTlas.SupportsD3D12NvapiHeaders,
	            .SupportsD3D12NvapiRuntime = rayTracing.Groups.PartitionedTlas.SupportsD3D12NvapiRuntime,
	            .SupportsD3D12DeviceInterface = rayTracing.Groups.PartitionedTlas.SupportsD3D12DeviceInterface,
	            .SupportsD3D12CommandListInterface = rayTracing.Groups.PartitionedTlas.SupportsD3D12CommandListInterface,
	            .SupportsD3D12PublicDxrProvider = rayTracing.Groups.PartitionedTlas.SupportsD3D12PublicDxrPartitionedTlas,
	            .SupportsD3D12PublicDxrHeaders = rayTracing.Groups.PartitionedTlas.SupportsD3D12PublicDxrHeaders,
	            .SupportsCpuPackedOperations = rayTracing.Groups.PartitionedTlas.SupportsCpuPackedOperations,
	            .SupportsGpuDrivenOperations = rayTracing.Groups.PartitionedTlas.SupportsGpuDrivenOperations,
	            .CapabilityStatusReason = rayTracing.Groups.PartitionedTlas.CapabilityStatusReason}};
}
