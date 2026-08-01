#include "PCH.h"

#include "RayTracing/RhiRayTracingDesc.h"

void PopulateStandardRayTracingCapabilityGroups(RhiRayTracingCapabilities& capabilities) noexcept
{
	capabilities.Groups.AccelerationStructures = RhiAccelerationStructureCapabilities{
	    .SupportsRayTracing = capabilities.SupportsRayTracing,
	    .SupportsInlineRayQuery = capabilities.SupportsInlineRayQuery,
	    .SupportsAccelerationStructureShaderBinding = capabilities.SupportsRayTracing,
	    .MaxTraceRecursionDepth = capabilities.MaxTraceRecursionDepth,
	    .MaxRayPayloadSizeInBytes = capabilities.MaxRayPayloadSizeInBytes,
	    .MaxRayAttributeSizeInBytes = capabilities.MaxRayAttributeSizeInBytes,
	    .ShaderGroupHandleSizeInBytes = capabilities.ShaderGroupHandleSizeInBytes,
	    .ShaderTableAlignmentInBytes = capabilities.ShaderTableAlignmentInBytes,
	    .ShaderTableRecordAlignmentInBytes = capabilities.ShaderTableRecordAlignmentInBytes,
	    .AccelerationStructureByteAlignment = capabilities.AccelerationStructureByteAlignment,
	    .ScratchBufferByteAlignment = capabilities.ScratchBufferByteAlignment};
	capabilities.Groups.ClassicTlas = RhiClassicTlasCapabilities{
	    .SupportsClassicTlasBuild = capabilities.SupportsRayTracing,
	    .SupportsClassicTlasUpdate = capabilities.SupportsRayTracing,
	    .SupportsGpuReadableInstanceBuffer = capabilities.SupportsRayTracing,
	    .InstanceDescSizeInBytes = capabilities.InstanceDescSizeInBytes};
}

const char* RhiRayTracingTopLevelProviderToString(ERhiRayTracingTopLevelProvider provider) noexcept
{
	switch (provider)
	{
		case ERhiRayTracingTopLevelProvider::ClassicTlas:
			return "ClassicTlas";
		case ERhiRayTracingTopLevelProvider::PartitionedTlas:
			return "PartitionedTlas";
		case ERhiRayTracingTopLevelProvider::None:
		default:
			return "None";
	}
}
