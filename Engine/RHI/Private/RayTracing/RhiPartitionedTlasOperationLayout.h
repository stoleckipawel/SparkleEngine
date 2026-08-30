#pragma once

#include "RayTracing/RhiPartitionedTlasDesc.h"

struct RhiPartitionedTlasNativeOperationLayout final
{
	std::uint64_t OperationCountSizeInBytes = 0;
	std::uint64_t OperationHeaderStrideInBytes = 0;
	std::uint64_t OperationHeaderAlignmentInBytes = 0;
	std::uint64_t InstanceWriteStrideInBytes = 0;
	std::uint64_t InstanceWriteAlignmentInBytes = 0;
	std::uint64_t InstanceUpdateStrideInBytes = 0;
	std::uint64_t InstanceUpdateAlignmentInBytes = 0;
	std::uint64_t PartitionTranslationStrideInBytes = 0;
	std::uint64_t PartitionTranslationAlignmentInBytes = 0;
	std::uint64_t BufferAlignmentInBytes = 0;
};

class RhiPartitionedTlasOperationLayout final
{
public:
	static RhiPartitionedTlasOperationBufferLayout Build(
	    std::uint32_t operationCount,
	    std::uint32_t instanceWriteCount,
	    std::uint32_t instanceUpdateCount,
	    std::uint32_t partitionTranslationCount,
	    const RhiPartitionedTlasNativeOperationLayout& nativeLayout) noexcept;

	static RhiGpuVirtualAddress ResolveArgumentAddress(
	    const RhiPartitionedTlasOperationHeader& operation,
	    RhiGpuVirtualAddress instanceWriteAddress,
	    RhiGpuVirtualAddress instanceUpdateAddress,
	    RhiGpuVirtualAddress partitionTranslationAddress) noexcept;
	static std::uint64_t ResolveArgumentStride(
	    const RhiPartitionedTlasOperationHeader& operation,
	    const RhiPartitionedTlasNativeOperationLayout& nativeLayout) noexcept;

	static std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept;
};
