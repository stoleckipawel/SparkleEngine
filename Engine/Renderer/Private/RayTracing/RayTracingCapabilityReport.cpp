#include "../PCH.h"
#include "RayTracing/RayTracingCapabilityReport.h"

#include "RHI/Public/Core/RhiCapabilities.h"

RayTracingCapabilityReport BuildRayTracingCapabilityReport(const RhiCapabilities& capabilities) noexcept
{
	const RhiRayTracingCapabilities& rayTracing = capabilities.RayTracing;
	const bool supportsPartitionedDescriptorTlas = rayTracing.Groups.PartitionedTlas.SupportsDescriptorAccess;
	return RayTracingCapabilityReport{
	    .SupportsRayTracing = rayTracing.SupportsRayTracing,
	    .TopLevelProvider =
	        RayTracingTopLevelProviderCapabilityReport{
	            .SelectedProvider = rayTracing.Groups.Provider.SelectedTopLevelProvider,
	            .SelectionReason = rayTracing.Groups.Provider.SelectedTopLevelProviderReason},
	    .PartitionedTlas = RayTracingPartitionedTlasCapabilityReport{
	        .Provider = rayTracing.Groups.PartitionedTlas.Provider,
	        .Supported = rayTracing.Groups.PartitionedTlas.Supported,
	        .SupportsDescriptor = supportsPartitionedDescriptorTlas,
	        .CapabilityStatusReason = rayTracing.Groups.PartitionedTlas.CapabilityStatusReason}};
}
