#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Memory/VulkanGpuAllocation.h"

#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"

VulkanGpuAllocationRecord::~VulkanGpuAllocationRecord() noexcept
{
	if (Owner != nullptr)
	{
		Owner->UnregisterAllocationRecord(*this);
	}

	if (Owner != nullptr && IsMapped && Allocation != nullptr)
	{
		Owner->UnmapAllocation(*this);
	}

	if (Owner != nullptr)
	{
		Owner->DestroyAllocation(*this);
		Owner = nullptr;
	}
}

RhiOwnedResourceHandle MakeVulkanOwnedResourceHandle(std::unique_ptr<VulkanGpuAllocationRecord> record) noexcept
{
	return RhiOwnedResourceHandle{record.release()};
}

std::unique_ptr<VulkanGpuAllocationRecord> TakeVulkanOwnedResourceHandle(RhiOwnedResourceHandle handle) noexcept
{
	return std::unique_ptr<VulkanGpuAllocationRecord>(static_cast<VulkanGpuAllocationRecord*>(handle.Value));
}

VulkanGpuAllocationRecord* GetVulkanGpuAllocationRecord(RhiOwnedResourceHandle handle) noexcept
{
	return static_cast<VulkanGpuAllocationRecord*>(handle.Value);
}

NativeResourceHandle GetVulkanNativeResource(VulkanGpuAllocationRecord& record) noexcept
{
	switch (record.ResourceKind)
	{
		case VulkanGpuAllocationResourceKind::Buffer:
			return NativeResourceHandle{record.Buffer};
		case VulkanGpuAllocationResourceKind::Image:
			return NativeResourceHandle{record.Image};
		case VulkanGpuAllocationResourceKind::Unknown:
		default:
			return {};
	}
}

void SetVulkanAllocationRecordDebugName(VulkanGpuAllocationRecord& record, std::wstring_view debugName) noexcept
{
	record.DebugName = debugName;
	if (record.Owner != nullptr)
	{
		record.Owner->SetAllocationDebugName(record, debugName);
	}
}