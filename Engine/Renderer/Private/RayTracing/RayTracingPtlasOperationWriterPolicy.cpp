#include "PCH.h"

#include "RayTracing/RayTracingPtlasOperationWriterPolicy.h"

bool RayTracingPtlasOperationWriterPolicyResolver::IsRecognizedPath(ERhiPartitionedTlasOperationWriterPath path) noexcept
{
	switch (path)
	{
		case ERhiPartitionedTlasOperationWriterPath::CpuPack:
		case ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack:
		case ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack:
			return true;
		case ERhiPartitionedTlasOperationWriterPath::None:
		default:
			return false;
	}
}

ERhiPartitionedTlasOperationWriterPath RayTracingPtlasOperationWriterPolicyResolver::ResolveRequestedPath() noexcept
{
	return ERhiPartitionedTlasOperationWriterPath::CpuPack;
}

const char* RayTracingPtlasOperationWriterPolicyResolver::ResolveUnsupportedReason(
    ERhiPartitionedTlasOperationWriterPath requestedPath) noexcept
{
	switch (requestedPath)
	{
		case ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack:
			return "ptlas-gpu-logical-dirty-writer-not-implemented";
		case ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack:
			return "ptlas-full-gpu-native-pack-backend-pack-shader-not-implemented";
		case ERhiPartitionedTlasOperationWriterPath::None:
			return "ptlas-operation-writer-none-requested";
		case ERhiPartitionedTlasOperationWriterPath::CpuPack:
		default:
			return "ptlas-operation-writer-cpu-pack-selected";
	}
}

RayTracingPtlasOperationWriterPolicy RayTracingPtlasOperationWriterPolicyResolver::ResolveForCapability(
    const RayTracingPartitionedTlasCapabilityReport& capabilityReport) noexcept
{
	RayTracingPtlasOperationWriterPolicy policy{};
	policy.RequestedPath = ResolveRequestedPath();
	policy.SelectedPath = policy.RequestedPath;

	if (policy.RequestedPath == ERhiPartitionedTlasOperationWriterPath::CpuPack)
	{
		policy.SelectionReason = capabilityReport.Supported && !capabilityReport.SupportsCpuPackedOperations
		                             ? "ptlas-cpu-pack-not-supported-by-provider"
		                             : "ptlas-operation-writer-cpu-pack-selected";
		policy.SelectedPath = capabilityReport.Supported && !capabilityReport.SupportsCpuPackedOperations
		                          ? ERhiPartitionedTlasOperationWriterPath::None
		                          : ERhiPartitionedTlasOperationWriterPath::CpuPack;
		return policy;
	}

	if (policy.RequestedPath == ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack)
	{
		const bool canUseGpuLogicalDirtyCpuNativePack =
		    capabilityReport.Supported &&
		    capabilityReport.SupportsGpuLogicalUpdateRecordWrites &&
		    capabilityReport.SupportsCpuPackedOperations;
		policy.SelectedPath = canUseGpuLogicalDirtyCpuNativePack
		                          ? ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack
		                          : ERhiPartitionedTlasOperationWriterPath::CpuPack;
		policy.SelectionReason = policy.SelectedPath == ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack
		                             ? "ptlas-gpu-logical-dirty-cpu-native-pack-selected"
		                             : ResolveUnsupportedReason(policy.RequestedPath);
		return policy;
	}

	const bool canUseFullGpuNativePack =
	    capabilityReport.Supported &&
	    capabilityReport.SupportsGpuDrivenOperations &&
	    capabilityReport.SupportsGpuLogicalUpdateRecordWrites &&
	    capabilityReport.SupportsGpuNativeOperationPacking;
	policy.SelectedPath = canUseFullGpuNativePack
	                          ? ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack
	                          : (capabilityReport.SupportsCpuPackedOperations ? ERhiPartitionedTlasOperationWriterPath::CpuPack
	                                                                          : ERhiPartitionedTlasOperationWriterPath::None);
	policy.SelectionReason = ResolveUnsupportedReason(policy.RequestedPath);
	return policy;
}
