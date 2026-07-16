#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Diagnostics/VulkanRenderDiagnostics.h"

#include "Commands/RenderCommandList.h"
#include "Device/RenderHardwareInterface.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"

#include "Core/Public/Strings/StringUtils.h"

#include <string>

class VulkanRenderObjectDiagnostics final : public RenderObjectDiagnostics
{
  public:
	explicit VulkanRenderObjectDiagnostics(VulkanRhi& rhi) noexcept : m_rhi(rhi) {}

	bool SupportsObjectNames() const noexcept override { return m_rhi.GetSetDebugUtilsObjectName() != nullptr; }

	void SetDebugName(NativeGraphicsDeviceHandle device, std::wstring_view debugName) noexcept override
	{
		SetDebugName(VK_OBJECT_TYPE_DEVICE, reinterpret_cast<std::uint64_t>(device.Value), debugName);
	}

	void SetDebugName(NativeGraphicsQueueHandle queue, std::wstring_view debugName) noexcept override
	{
		SetDebugName(VK_OBJECT_TYPE_QUEUE, reinterpret_cast<std::uint64_t>(queue.Value), debugName);
	}

	void SetDebugName(const RenderCommandList& commandList, std::wstring_view debugName) noexcept override
	{
		SetDebugName(
		    VK_OBJECT_TYPE_COMMAND_BUFFER,
		    reinterpret_cast<std::uint64_t>(commandList.GetNativeHandle(
		                                        RhiNativeInteropRequest{
		                                            .Consumer = ERhiNativeInteropConsumer::Validation,
		                                            .Reason = "Assign Vulkan command buffer debug name"})
		                                        .Value),
		    debugName);
	}

	void SetDebugName(RhiResourceHandle resource, std::wstring_view debugName) noexcept override
	{
		SetDebugName(VK_OBJECT_TYPE_UNKNOWN, reinterpret_cast<std::uint64_t>(resource.Value), debugName);
	}

	void SetDebugName(RhiOwnedMemoryBlockHandle memoryBlock, std::wstring_view debugName) noexcept override
	{
		VulkanGpuMemoryBlockRecord* const record = GetVulkanGpuMemoryBlockRecord(memoryBlock);
		if (record != nullptr)
		{
			SetVulkanMemoryBlockRecordDebugName(*record, debugName);
		}
	}

	void SetDebugName(RhiOwnedResourceHandle resource, std::wstring_view debugName) noexcept override
	{
		VulkanGpuAllocationRecord* const record = GetVulkanGpuAllocationRecord(resource);
		if (record != nullptr)
		{
			SetVulkanAllocationRecordDebugName(*record, debugName);
		}
	}

  private:
	void SetDebugName(VkObjectType objectType, std::uint64_t objectHandle, std::wstring_view debugName) noexcept
	{
		if (objectHandle == 0 || debugName.empty())
		{
			return;
		}

		const std::string narrowName = Strings::ToNarrow(debugName);
		(void)VulkanDebugNames::SetObjectName(
		    m_rhi.GetSetDebugUtilsObjectName(),
		    m_rhi.GetDevice(),
		    objectType,
		    objectHandle,
		    narrowName);
	}

	VulkanRhi& m_rhi;
};

class VulkanRenderMessageDiagnostics final : public RenderMessageDiagnostics
{
  public:
	explicit VulkanRenderMessageDiagnostics(VulkanRhi& rhi) noexcept : m_rhi(rhi) {}

	bool SupportsDebugMessages() const noexcept override { return m_rhi.IsValidationEnabled(); }

	bool TryPopMessage(RhiDiagnosticMessage& outMessage) noexcept override { return m_rhi.TryPopDiagnosticMessage(outMessage); }

	void ClearMessages() noexcept override { m_rhi.ClearDiagnosticMessages(); }

  private:
	VulkanRhi& m_rhi;
};

class VulkanRenderMemoryDiagnostics final : public RenderMemoryDiagnostics
{
  public:
	explicit VulkanRenderMemoryDiagnostics(VulkanGpuMemoryAllocator& allocator) noexcept : m_allocator(allocator) {}

	bool SupportsBudgetQueries() const noexcept override { return m_allocator.SupportsBudgetQueries(); }

	bool SupportsDelayedDestructionTracking() const noexcept override { return true; }

	RhiMemoryUsageSnapshot GetLatestMemorySnapshot() const override { return m_allocator.CreateMemoryUsageSnapshot(); }

  private:
	VulkanGpuMemoryAllocator& m_allocator;
};

class VulkanRenderDiagnostics final : public RenderDiagnostics
{
  public:
	VulkanRenderDiagnostics(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept :
	    m_rhi(rhi), m_objectDiagnostics(rhi), m_messageDiagnostics(rhi), m_memoryDiagnostics(memoryAllocator)
	{
	}

	RhiDiagnosticsCapabilities GetCapabilities() const noexcept override
	{
		const bool supportsGpuEvents = m_objectDiagnostics.SupportsObjectNames() && m_rhi.GetCmdBeginDebugUtilsLabel() != nullptr &&
		                               m_rhi.GetCmdEndDebugUtilsLabel() != nullptr && m_rhi.GetCmdInsertDebugUtilsLabel() != nullptr;
		return RhiDiagnosticsCapabilities{
		    .SupportsObjectNames = m_objectDiagnostics.SupportsObjectNames(),
		    .SupportsGpuEvents = supportsGpuEvents,
		    .SupportsTimestampQueries = false,
		    .SupportsDebugMessages = m_messageDiagnostics.SupportsDebugMessages(),
		    .SupportsLiveObjectReports = false,
		    .SupportsCrashDiagnostics = false,
		    .SupportsMemoryDiagnostics = true,
		    .SupportsMemoryBudgetQueries = m_memoryDiagnostics.SupportsBudgetQueries()};
	}

	RenderObjectDiagnostics& GetObjectDiagnostics() noexcept override { return m_objectDiagnostics; }

	const RenderObjectDiagnostics& GetObjectDiagnostics() const noexcept override { return m_objectDiagnostics; }

	RenderTimingDiagnostics* GetTimingDiagnostics() noexcept override { return nullptr; }

	const RenderTimingDiagnostics* GetTimingDiagnostics() const noexcept override { return nullptr; }

	RenderMessageDiagnostics* GetMessageDiagnostics() noexcept override
	{
		return m_messageDiagnostics.SupportsDebugMessages() ? &m_messageDiagnostics : nullptr;
	}

	const RenderMessageDiagnostics* GetMessageDiagnostics() const noexcept override
	{
		return m_messageDiagnostics.SupportsDebugMessages() ? &m_messageDiagnostics : nullptr;
	}

	RenderFailureDiagnostics* GetFailureDiagnostics() noexcept override { return nullptr; }

	const RenderFailureDiagnostics* GetFailureDiagnostics() const noexcept override { return nullptr; }

	RenderMemoryDiagnostics* GetMemoryDiagnostics() noexcept override { return &m_memoryDiagnostics; }

	const RenderMemoryDiagnostics* GetMemoryDiagnostics() const noexcept override { return &m_memoryDiagnostics; }

  private:
	VulkanRhi& m_rhi;
	VulkanRenderObjectDiagnostics m_objectDiagnostics;
	VulkanRenderMessageDiagnostics m_messageDiagnostics;
	VulkanRenderMemoryDiagnostics m_memoryDiagnostics;
};

std::unique_ptr<RenderDiagnostics> CreateVulkanRenderDiagnostics(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator)
{
	return std::make_unique<VulkanRenderDiagnostics>(rhi, memoryAllocator);
}
