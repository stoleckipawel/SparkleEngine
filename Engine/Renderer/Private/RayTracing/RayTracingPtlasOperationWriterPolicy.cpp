#include "PCH.h"

#include "RayTracing/RayTracingPtlasOperationWriterPolicy.h"

#include "Debug/RendererCVars.h"

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
	const ERhiPartitionedTlasOperationWriterPath requestedPath = CVarRayTracingPtlasOperationWriterPath.Get();
	return IsRecognizedPath(requestedPath) ? requestedPath : ERhiPartitionedTlasOperationWriterPath::CpuPack;
}

const char* RayTracingPtlasOperationWriterPolicyResolver::ResolveUnsupportedReason(
    ERhiPartitionedTlasOperationWriterPath requestedPath) noexcept
{
	switch (requestedPath)
	{
		case ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack:
			return "ptlas-gpu-logical-dirty-writer-not-implemented";
		case ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack:
			return "ptlas-full-gpu-native-pack-not-implemented";
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

	policy.SelectedPath = capabilityReport.SupportsCpuPackedOperations ? ERhiPartitionedTlasOperationWriterPath::CpuPack
	                                                                   : ERhiPartitionedTlasOperationWriterPath::None;
	policy.SelectionReason = ResolveUnsupportedReason(policy.RequestedPath);
	return policy;
}
