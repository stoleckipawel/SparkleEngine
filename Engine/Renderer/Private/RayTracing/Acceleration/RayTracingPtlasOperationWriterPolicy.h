#pragma once

#include "RayTracing/RayTracingCapabilityReport.h"
#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"

struct RayTracingPtlasOperationWriterPolicy final
{
	ERhiPartitionedTlasOperationWriterPath RequestedPath = ERhiPartitionedTlasOperationWriterPath::CpuPack;
	ERhiPartitionedTlasOperationWriterPath SelectedPath = ERhiPartitionedTlasOperationWriterPath::CpuPack;
	const char* SelectionReason = "ptlas-operation-writer-cpu-pack-selected";
};

class RayTracingPtlasOperationWriterPolicyResolver final
{
  public:
	static RayTracingPtlasOperationWriterPolicy ResolveForCapability(
	    const RayTracingPartitionedTlasCapabilityReport& capabilityReport) noexcept;
	static ERhiPartitionedTlasOperationWriterPath ResolveRequestedPath() noexcept;
	static const char* ResolveUnsupportedReason(ERhiPartitionedTlasOperationWriterPath requestedPath) noexcept;

  private:
	static bool IsRecognizedPath(ERhiPartitionedTlasOperationWriterPath path) noexcept;
};
