#include "PCH.h"

#include "RayTracing/RhiPartitionedTlasOperationLayout.h"

RhiPartitionedTlasOperationBufferLayout RhiPartitionedTlasOperationLayout::Build(
    std::uint32_t operationCount,
    std::uint32_t instanceWriteCount,
    std::uint32_t instanceUpdateCount,
    std::uint32_t partitionTranslationCount,
    const RhiPartitionedTlasNativeOperationLayout& nativeLayout) noexcept
{
	RhiPartitionedTlasOperationBufferLayout layout{
	    .OperationCountOffsetInBytes = 0,
	    .OperationHeaderStrideInBytes = nativeLayout.OperationHeaderStrideInBytes,
	    .InstanceWriteStrideInBytes = nativeLayout.InstanceWriteStrideInBytes,
	    .InstanceUpdateStrideInBytes = nativeLayout.InstanceUpdateStrideInBytes,
	    .PartitionTranslationStrideInBytes = nativeLayout.PartitionTranslationStrideInBytes};

	layout.OperationHeadersOffsetInBytes = AlignUp(nativeLayout.OperationCountSizeInBytes, nativeLayout.OperationHeaderAlignmentInBytes);
	layout.InstanceWriteRecordsOffsetInBytes = AlignUp(
	    layout.OperationHeadersOffsetInBytes + layout.OperationHeaderStrideInBytes * operationCount,
	    nativeLayout.InstanceWriteAlignmentInBytes);

	std::uint64_t cursor = layout.InstanceWriteRecordsOffsetInBytes + layout.InstanceWriteStrideInBytes * instanceWriteCount;
	layout.InstanceUpdateRecordsOffsetInBytes =
	    instanceUpdateCount > 0 ? AlignUp(cursor, nativeLayout.InstanceUpdateAlignmentInBytes) : cursor;
	cursor = layout.InstanceUpdateRecordsOffsetInBytes + layout.InstanceUpdateStrideInBytes * instanceUpdateCount;
	layout.PartitionTranslationRecordsOffsetInBytes =
	    partitionTranslationCount > 0 ? AlignUp(cursor, nativeLayout.PartitionTranslationAlignmentInBytes) : cursor;
	cursor = layout.PartitionTranslationRecordsOffsetInBytes + layout.PartitionTranslationStrideInBytes * partitionTranslationCount;
	layout.TotalSizeInBytes = AlignUp(cursor, nativeLayout.BufferAlignmentInBytes);
	return layout;
}

RhiGpuVirtualAddress RhiPartitionedTlasOperationLayout::ResolveArgumentAddress(
    const RhiPartitionedTlasOperationHeader& operation,
    RhiGpuVirtualAddress instanceWriteAddress,
    RhiGpuVirtualAddress instanceUpdateAddress,
    RhiGpuVirtualAddress partitionTranslationAddress) noexcept
{
	if (operation.ArgumentData != 0)
	{
		return operation.ArgumentData;
	}

	switch (operation.Type)
	{
		case ERhiPartitionedTlasOperationType::UpdateInstance:
			return instanceUpdateAddress;
		case ERhiPartitionedTlasOperationType::WritePartitionTranslation:
			return partitionTranslationAddress;
		case ERhiPartitionedTlasOperationType::WriteInstance:
		default:
			return instanceWriteAddress;
	}
}

std::uint64_t RhiPartitionedTlasOperationLayout::ResolveArgumentStride(
    const RhiPartitionedTlasOperationHeader& operation,
    const RhiPartitionedTlasNativeOperationLayout& nativeLayout) noexcept
{
	if (operation.ArgumentStrideInBytes != 0)
	{
		return operation.ArgumentStrideInBytes;
	}

	switch (operation.Type)
	{
		case ERhiPartitionedTlasOperationType::UpdateInstance:
			return nativeLayout.InstanceUpdateStrideInBytes;
		case ERhiPartitionedTlasOperationType::WritePartitionTranslation:
			return nativeLayout.PartitionTranslationStrideInBytes;
		case ERhiPartitionedTlasOperationType::WriteInstance:
		default:
			return nativeLayout.InstanceWriteStrideInBytes;
	}
}

std::uint64_t RhiPartitionedTlasOperationLayout::AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept
{
	return alignment == 0 ? value : ((value + alignment - 1u) / alignment) * alignment;
}
