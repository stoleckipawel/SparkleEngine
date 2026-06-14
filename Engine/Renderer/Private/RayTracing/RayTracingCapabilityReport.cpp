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
	return SupportsRayTracing && SupportsInlineRayQuery && HasAccelerationStructureAlignment && HasScratchBufferAlignment &&
	       HasInstanceDescSize;
}

const char* RayTracingCapabilityReport::GetInlineRayQueryShadowUnavailableReason() const noexcept
{
	if (!SupportsRayTracing)
	{
		return "ray-tracing-unsupported";
	}
	if (!SupportsInlineRayQuery)
	{
		return "inline-ray-query-unsupported";
	}
	if (!HasAccelerationStructureAlignment)
	{
		return "missing-acceleration-structure-alignment";
	}
	if (!HasScratchBufferAlignment)
	{
		return "missing-scratch-buffer-alignment";
	}
	if (!HasInstanceDescSize)
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
	    "d3d12PublicDxr={} d3d12PublicDxrHeaders={} gpuDrivenOps={} partitionedReason={}",
	    RhiBackendApiToString(report.BackendApi),
	    BoolToString(report.SupportsRayTracing),
	    BoolToString(report.SupportsInlineRayQuery),
	    report.AccelerationStructureByteAlignment,
	    report.ScratchBufferByteAlignment,
	    report.InstanceDescSizeInBytes,
	    report.MaxTraceRecursionDepth,
	    report.MaxRayPayloadSizeInBytes,
	    report.MaxRayAttributeSizeInBytes,
	    BoolToString(report.CanUseInlineRayQueryShadows()),
	    report.GetInlineRayQueryShadowUnavailableReason(),
	    RhiRayTracingTopLevelProviderToString(report.SelectedTopLevelProvider),
	    report.SelectedTopLevelProviderReason,
	    RhiPartitionedTlasProviderToString(report.PartitionedTlasProvider),
	    BoolToString(report.SupportsPartitionedTlas),
	    BoolToString(report.PartitionedTlasRequiresNvidiaDevice),
	    BoolToString(report.PartitionedTlasRunsOnNvidiaDevice),
	    BoolToString(report.SupportsVulkanNativePartitionedTlas),
	    BoolToString(report.SupportsVulkanPartitionedTlasExtension),
	    BoolToString(report.SupportsVulkanPartitionedTlasFeatureQuery),
	    BoolToString(report.SupportsVulkanPartitionedTlasFunctionLoading),
	    BoolToString(report.SupportsVulkanPartitionedTlasDescriptorPath),
	    BoolToString(report.SupportsD3D12NvapiPartitionedTlas),
	    BoolToString(report.SupportsD3D12NvapiPartitionedTlasHeaders),
	    BoolToString(report.SupportsD3D12NvapiPartitionedTlasRuntime),
	    BoolToString(report.SupportsD3D12PartitionedTlasDeviceInterface),
	    BoolToString(report.SupportsD3D12PartitionedTlasCommandListInterface),
	    BoolToString(report.SupportsD3D12PublicDxrPartitionedTlas),
	    BoolToString(report.SupportsD3D12PublicDxrPartitionedTlasHeaders),
	    BoolToString(report.SupportsGpuDrivenPartitionedTlasOperations),
	    report.PartitionedTlasCapabilityStatusReason);
}

RayTracingCapabilityReport RayTracingCapabilityReporter::Build(
    ERhiBackendApi backendApi,
    const RhiRayTracingCapabilities& rayTracing) noexcept
{
	return RayTracingCapabilityReport{
	    .BackendApi = backendApi,
	    .SupportsRayTracing = rayTracing.SupportsRayTracing,
	    .SupportsInlineRayQuery = rayTracing.SupportsInlineRayQuery,
	    .HasAccelerationStructureAlignment = rayTracing.AccelerationStructureByteAlignment != 0,
	    .HasScratchBufferAlignment = rayTracing.ScratchBufferByteAlignment != 0,
	    .HasInstanceDescSize = rayTracing.InstanceDescSizeInBytes != 0,
	    .MaxTraceRecursionDepth = rayTracing.MaxTraceRecursionDepth,
	    .MaxRayPayloadSizeInBytes = rayTracing.MaxRayPayloadSizeInBytes,
	    .MaxRayAttributeSizeInBytes = rayTracing.MaxRayAttributeSizeInBytes,
	    .AccelerationStructureByteAlignment = rayTracing.AccelerationStructureByteAlignment,
	    .ScratchBufferByteAlignment = rayTracing.ScratchBufferByteAlignment,
	    .InstanceDescSizeInBytes = rayTracing.InstanceDescSizeInBytes,
	    .SelectedTopLevelProvider = rayTracing.Groups.Provider.SelectedTopLevelProvider,
	    .SelectedTopLevelProviderReason = rayTracing.Groups.Provider.SelectedTopLevelProviderReason,
	    .PartitionedTlasProvider = rayTracing.Groups.PartitionedTlas.Provider,
	    .SupportsPartitionedTlas = rayTracing.Groups.PartitionedTlas.Supported,
	    .PartitionedTlasRequiresNvidiaDevice = rayTracing.Groups.PartitionedTlas.RequiresNvidiaDevice,
	    .PartitionedTlasRunsOnNvidiaDevice = rayTracing.Groups.PartitionedTlas.RunsOnNvidiaDevice,
	    .SupportsVulkanNativePartitionedTlas = rayTracing.Groups.PartitionedTlas.SupportsVulkanNativePartitionedAccelerationStructure,
	    .SupportsVulkanPartitionedTlasExtension = rayTracing.Groups.PartitionedTlas.SupportsVulkanExtension,
	    .SupportsVulkanPartitionedTlasFeatureQuery = rayTracing.Groups.PartitionedTlas.SupportsVulkanFeatureQuery,
	    .SupportsVulkanPartitionedTlasFunctionLoading = rayTracing.Groups.PartitionedTlas.SupportsVulkanFunctionLoading,
	    .SupportsVulkanPartitionedTlasDescriptorPath = rayTracing.Groups.PartitionedTlas.SupportsVulkanDescriptorPath,
	    .SupportsD3D12NvapiPartitionedTlas = rayTracing.Groups.PartitionedTlas.SupportsD3D12NvapiPartitionedTlas,
	    .SupportsD3D12NvapiPartitionedTlasHeaders = rayTracing.Groups.PartitionedTlas.SupportsD3D12NvapiHeaders,
	    .SupportsD3D12NvapiPartitionedTlasRuntime = rayTracing.Groups.PartitionedTlas.SupportsD3D12NvapiRuntime,
	    .SupportsD3D12PartitionedTlasDeviceInterface = rayTracing.Groups.PartitionedTlas.SupportsD3D12DeviceInterface,
	    .SupportsD3D12PartitionedTlasCommandListInterface = rayTracing.Groups.PartitionedTlas.SupportsD3D12CommandListInterface,
	    .SupportsD3D12PublicDxrPartitionedTlas = rayTracing.Groups.PartitionedTlas.SupportsD3D12PublicDxrPartitionedTlas,
	    .SupportsD3D12PublicDxrPartitionedTlasHeaders = rayTracing.Groups.PartitionedTlas.SupportsD3D12PublicDxrHeaders,
	    .SupportsGpuDrivenPartitionedTlasOperations = rayTracing.Groups.PartitionedTlas.SupportsGpuDrivenOperations,
	    .PartitionedTlasCapabilityStatusReason = rayTracing.Groups.PartitionedTlas.CapabilityStatusReason};
}
