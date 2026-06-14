#include "PCH.h"

#include "RayTracing/RhiPartitionedTlasDesc.h"

#include <cstdint>

RhiPartitionedTlasInstanceFlags operator|(
    RhiPartitionedTlasInstanceFlags lhs,
    RhiPartitionedTlasInstanceFlags rhs) noexcept
{
	return static_cast<RhiPartitionedTlasInstanceFlags>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

bool HasFlag(RhiPartitionedTlasInstanceFlags flags, RhiPartitionedTlasInstanceFlags flag) noexcept
{
	return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(flag)) != 0;
}

RhiPartitionedTlasLogicalUpdateFlags operator|(
    RhiPartitionedTlasLogicalUpdateFlags lhs,
    RhiPartitionedTlasLogicalUpdateFlags rhs) noexcept
{
	return static_cast<RhiPartitionedTlasLogicalUpdateFlags>(
	    static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

bool HasFlag(RhiPartitionedTlasLogicalUpdateFlags flags, RhiPartitionedTlasLogicalUpdateFlags flag) noexcept
{
	return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(flag)) != 0;
}

const char* RhiPartitionedTlasProviderToString(ERhiPartitionedTlasProvider provider) noexcept
{
	switch (provider)
	{
		case ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure:
			return "VulkanNvPartitionedAccelerationStructure";
		case ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas:
			return "D3D12NvapiPartitionedTlas";
		case ERhiPartitionedTlasProvider::D3D12PublicDxrRtasOperations:
			return "D3D12PublicDxrRtasOperations";
		case ERhiPartitionedTlasProvider::None:
		default:
			return "None";
	}
}

const char* RhiPartitionedTlasOperationWriterPathToString(ERhiPartitionedTlasOperationWriterPath writerPath) noexcept
{
	switch (writerPath)
	{
		case ERhiPartitionedTlasOperationWriterPath::CpuPack:
			return "CpuPack";
		case ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack:
			return "GpuLogicalDirtyCpuNativePack";
		case ERhiPartitionedTlasOperationWriterPath::FullGpuNativePack:
			return "FullGpuNativePack";
		case ERhiPartitionedTlasOperationWriterPath::None:
		default:
			return "None";
	}
}
