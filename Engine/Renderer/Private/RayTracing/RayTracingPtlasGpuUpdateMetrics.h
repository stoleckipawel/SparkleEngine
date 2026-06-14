#pragma once

#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"

#include <cstdint>

struct RayTracingPtlasGpuUpdateMetrics final
{
	ERhiPartitionedTlasOperationWriterPath SelectedWriterPath = ERhiPartitionedTlasOperationWriterPath::CpuPack;
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
