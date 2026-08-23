#include "../PCH.h"
#include "RayTracing/RayTracingCapabilityReport.h"

#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Core/RhiBackendSelection.h"

bool RayTracingCapabilityReport::CanUseInlineRayQueryShadows() const noexcept
{
	const bool hasAccelerationStructureContract = Core.SupportsRayTracing && Core.SupportsInlineRayQuery &&
	    AccelerationStructures.HasAccelerationStructureAlignment && AccelerationStructures.HasScratchBufferAlignment &&
	    AccelerationStructures.HasInstanceDescSize;
	if (!hasAccelerationStructureContract)
	{
		return false;
	}

	if (TopLevelProvider.SelectedProvider == ERhiRayTracingTopLevelProvider::ClassicTlas)
	{
		return true;
	}

	return TopLevelProvider.SelectedProvider == ERhiRayTracingTopLevelProvider::PartitionedTlas && PartitionedTlas.Supported &&
	       PartitionedTlas.SupportsDescriptor;
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
	if (TopLevelProvider.SelectedProvider == ERhiRayTracingTopLevelProvider::None)
	{
		return "top-level-provider-unavailable";
	}
	if (TopLevelProvider.SelectedProvider == ERhiRayTracingTopLevelProvider::PartitionedTlas &&
	    (!PartitionedTlas.Supported || !PartitionedTlas.SupportsDescriptor))
	{
		return "partitioned-tlas-semantic-binding-unavailable";
	}
	return "available";
}

RayTracingCapabilityReport RayTracingCapabilityReporter::Build(const RhiCapabilities& capabilities) noexcept
{
	return BuildFromCapabilities(capabilities);
}

RayTracingCapabilityReport RayTracingCapabilityReporter::BuildFromCapabilities(const RhiCapabilities& capabilities) noexcept
{
	const ERhiBackendApi backendApi = capabilities.BackendApi;
	const RhiRayTracingCapabilities& rayTracing = capabilities.RayTracing;
	const bool supportsPartitionedDescriptorTlas = rayTracing.Groups.PartitionedTlas.SupportsDescriptorAccess;
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
	            .SupportsDescriptor = supportsPartitionedDescriptorTlas,
	            .CapabilityStatusReason = rayTracing.Groups.PartitionedTlas.CapabilityStatusReason},
	    .MaterialTextureTable = BuildMaterialTextureTableCapabilityReport(capabilities)};
}
