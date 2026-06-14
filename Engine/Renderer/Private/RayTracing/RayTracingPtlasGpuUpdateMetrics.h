#pragma once

#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"

#include <cstdint>

struct RayTracingPtlasGpuUpdateMetrics final
{
	ERhiPartitionedTlasOperationWriterPath RequestedWriterPath = ERhiPartitionedTlasOperationWriterPath::CpuPack;
	ERhiPartitionedTlasOperationWriterPath SelectedWriterPath = ERhiPartitionedTlasOperationWriterPath::CpuPack;
	const char* WriterSelectionReason = "ptlas-operation-writer-cpu-pack-selected";
	std::uint32_t LogicalUpdateCount = 0;
	std::uint32_t NativeOperationCount = 0;
	std::uint32_t ValidationMismatchCount = 0;
	bool FullGpuNativePackSupported = false;
	bool FullGpuNativePackSubmitted = false;
	double CpuPackMilliseconds = 0.0;
	double GpuDirtyDetectionMilliseconds = 0.0;
	double GpuNativePackMilliseconds = 0.0;
	double PtlasUpdateGpuMilliseconds = 0.0;
};
