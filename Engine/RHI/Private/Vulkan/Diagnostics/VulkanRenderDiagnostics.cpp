#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Diagnostics/VulkanRenderDiagnostics.h"

#include "Commands/RenderCommandList.h"
#include "Diagnostics/RhiDiagnosticsComposition.h"
#include "Diagnostics/RhiTimestampQueryAllocator.h"
#include "Interop/RhiInteropService.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"

#include "Core/Public/Strings/StringUtils.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

static const auto g_vulkanRenderDiagnosticsLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Diagnostics");

class VulkanRenderObjectDiagnostics final : public RenderObjectDiagnostics
{
  public:
	explicit VulkanRenderObjectDiagnostics(VulkanRhi& rhi) noexcept : m_rhi(rhi) {}

	bool SupportsObjectNames() const noexcept override { return m_rhi.GetSetDebugUtilsObjectName() != nullptr; }

	void SetDebugName(const RenderCommandList& commandList, std::wstring_view debugName) noexcept override
	{
		SetDebugName(
		    VK_OBJECT_TYPE_COMMAND_BUFFER,
		    reinterpret_cast<std::uint64_t>(commandList.GetNativeHandle(
		                                        RhiNativeInteropRequest{
		                                            .Consumer = ERhiNativeInteropConsumer::Diagnostics,
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

class VulkanRenderTimingDiagnostics final : public RenderTimingDiagnostics
{
  public:
	explicit VulkanRenderTimingDiagnostics(VulkanRhi& rhi) noexcept :
	    m_rhi(rhi), m_queryAllocator(static_cast<std::uint32_t>(RhiQueueTypeCount), kQueriesPerQueue)
	{
		Initialize();
	}

	~VulkanRenderTimingDiagnostics() noexcept override
	{
		if (m_rhi.GetDevice() == VK_NULL_HANDLE)
		{
			return;
		}
		for (QueueTimingState& queueState : m_queueStates)
		{
			if (queueState.QueryPool != VK_NULL_HANDLE)
			{
				vkDestroyQueryPool(m_rhi.GetDevice(), queueState.QueryPool, nullptr);
				queueState.QueryPool = VK_NULL_HANDLE;
			}
		}
	}

	bool SupportsTimestampQueries() const noexcept override
	{
		return m_queueStates[RhiQueueTypeToIndex(ERhiQueueType::Graphics)].QueryPool != VK_NULL_HANDLE;
	}

	RhiTimestampQueryHandle AllocateTimestampQuery(ERhiQueueType queueType) override
	{
		if (!IsRhiQueueTypeValid(queueType))
		{
			Diagnostics::Fatal(g_vulkanRenderDiagnosticsLogger, __FILE__, __LINE__, "Timestamp query requested for an invalid Vulkan queue.");
		}

		const std::uint32_t poolIndex = static_cast<std::uint32_t>(RhiQueueTypeToIndex(queueType));
		QueueTimingState& queueState = m_queueStates[poolIndex];
		if (queueState.QueryPool == VK_NULL_HANDLE)
		{
			Diagnostics::Fatal(
			    g_vulkanRenderDiagnosticsLogger,
			    __FILE__,
			    __LINE__,
			    std::string("Vulkan queue does not support timestamp queries: ") + RhiQueueTypeToString(queueType));
		}

		const RhiTimestampQueryHandle query = m_queryAllocator.Allocate(poolIndex);
		const RhiTimestampQueryLocation location = m_queryAllocator.Resolve(query);
		std::lock_guard lock(m_queryPoolMutex);
		vkResetQueryPool(m_rhi.GetDevice(), queueState.QueryPool, location.QueryIndex, 1);
		return query;
	}

	void ReleaseTimestampQuery(RhiTimestampQueryHandle query) noexcept override
	{
		m_queryAllocator.Release(query);
	}

	bool WriteTimestamp(RenderCommandList& commandList, RhiTimestampQueryHandle query) noexcept override
	{
		const RhiTimestampQueryLocation location = m_queryAllocator.Resolve(query);
		const ERhiQueueType queueType = static_cast<ERhiQueueType>(location.PoolIndex);
		if (commandList.GetQueueType() != queueType)
		{
			Diagnostics::Fatal(g_vulkanRenderDiagnosticsLogger, __FILE__, __LINE__, "Vulkan timestamp query was written on a different queue than it was allocated for.");
		}

		const VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(
		    commandList.GetNativeHandle(
		                   RhiNativeInteropRequest{
		                       .Consumer = ERhiNativeInteropConsumer::Diagnostics,
		                       .Reason = "Write Vulkan timestamp query"})
		        .Value);
		if (commandBuffer == VK_NULL_HANDLE)
		{
			Diagnostics::Fatal(g_vulkanRenderDiagnosticsLogger, __FILE__, __LINE__, "Vulkan timestamp query has no native command buffer.");
		}

		vkCmdWriteTimestamp2(
		    commandBuffer,
		    VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		    m_queueStates[location.PoolIndex].QueryPool,
		    location.QueryIndex);
		return true;
	}

	bool TryResolveTimestamp(RhiTimestampQueryHandle query, std::uint64_t& outTicks) const noexcept override
	{
		const RhiTimestampQueryLocation location = m_queryAllocator.Resolve(query);
		const QueueTimingState& queueState = m_queueStates[location.PoolIndex];
		std::lock_guard lock(m_queryPoolMutex);
		const VkResult result = vkGetQueryPoolResults(
		    m_rhi.GetDevice(),
		    queueState.QueryPool,
		    location.QueryIndex,
		    1,
		    sizeof(outTicks),
		    &outTicks,
		    sizeof(outTicks),
		    VK_QUERY_RESULT_64_BIT);
		if (result == VK_NOT_READY)
		{
			return false;
		}
		if (!VulkanResult::Succeeded(result))
		{
			Diagnostics::Fatal(
			    g_vulkanRenderDiagnosticsLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkGetQueryPoolResults", result));
		}
		if (queueState.TimestampValidBits < 64)
		{
			outTicks &= (std::uint64_t{1} << queueState.TimestampValidBits) - 1;
		}
		return true;
	}

	double GetTimestampPeriodNanoseconds(RhiTimestampQueryHandle) const noexcept override { return m_timestampPeriodNanoseconds; }
	std::uint32_t GetTimestampValidBits(RhiTimestampQueryHandle query) const noexcept override
	{
		return m_queueStates[m_queryAllocator.Resolve(query).PoolIndex].TimestampValidBits;
	}

  private:
	static constexpr std::uint32_t kQueriesPerQueue = 8192;

	struct QueueTimingState final
	{
		VkQueryPool QueryPool = VK_NULL_HANDLE;
		std::uint32_t TimestampValidBits = 0;
	};

	void Initialize() noexcept
	{
		if (m_rhi.GetPhysicalDevice() == VK_NULL_HANDLE || m_rhi.GetDevice() == VK_NULL_HANDLE)
		{
			Diagnostics::Fatal(g_vulkanRenderDiagnosticsLogger, __FILE__, __LINE__, "Cannot initialize Vulkan timing without a physical device and logical device.");
		}

		VkPhysicalDeviceProperties physicalDeviceProperties = {};
		vkGetPhysicalDeviceProperties(m_rhi.GetPhysicalDevice(), &physicalDeviceProperties);

		std::uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_rhi.GetPhysicalDevice(), &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_rhi.GetPhysicalDevice(), &queueFamilyCount, queueFamilyProperties.data());
		if (physicalDeviceProperties.limits.timestampPeriod <= 0.0f)
		{
			Diagnostics::Fatal(g_vulkanRenderDiagnosticsLogger, __FILE__, __LINE__, "Vulkan physical device reported an invalid timestamp period.");
		}

		const VkQueryPoolCreateInfo queryPoolCreateInfo{
		    .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		    .pNext = nullptr,
		    .flags = 0,
		    .queryType = VK_QUERY_TYPE_TIMESTAMP,
		    .queryCount = kQueriesPerQueue,
		    .pipelineStatistics = 0};
		for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
		{
			const ERhiQueueType queueType = static_cast<ERhiQueueType>(queueIndex);
			const std::uint32_t familyIndex = m_rhi.GetQueueFamilyIndex(queueType);
			if (familyIndex >= queueFamilyProperties.size())
			{
				Diagnostics::Fatal(g_vulkanRenderDiagnosticsLogger, __FILE__, __LINE__, "Vulkan timing addressed an invalid queue family.");
			}

			QueueTimingState& queueState = m_queueStates[queueIndex];
			queueState.TimestampValidBits = queueFamilyProperties[familyIndex].timestampValidBits;
			if (queueState.TimestampValidBits == 0)
			{
				continue;
			}
			const VkResult result = vkCreateQueryPool(m_rhi.GetDevice(), &queryPoolCreateInfo, nullptr, &queueState.QueryPool);
			if (!VulkanResult::Succeeded(result))
			{
				Diagnostics::Fatal(
				    g_vulkanRenderDiagnosticsLogger,
				    __FILE__,
				    __LINE__,
				    VulkanResult::FormatFailure("vkCreateQueryPool", result));
			}
			const std::string queryPoolName = std::string("VulkanTimestampQueryPool_") + RhiQueueTypeToString(queueType);
			(void)VulkanDebugNames::SetObjectName(
			    m_rhi.GetSetDebugUtilsObjectName(),
			    m_rhi.GetDevice(),
			    VK_OBJECT_TYPE_QUERY_POOL,
			    reinterpret_cast<std::uint64_t>(queueState.QueryPool),
			    queryPoolName);
		}

		m_timestampPeriodNanoseconds = static_cast<double>(physicalDeviceProperties.limits.timestampPeriod);
	}

	VulkanRhi& m_rhi;
	RhiTimestampQueryAllocator m_queryAllocator;
	std::array<QueueTimingState, RhiQueueTypeCount> m_queueStates;
	mutable std::mutex m_queryPoolMutex;
	double m_timestampPeriodNanoseconds = 0.0;
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

std::unique_ptr<RenderDiagnostics> CreateVulkanRenderDiagnostics(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator)
{
	const bool supportsGpuEvents = rhi.GetSetDebugUtilsObjectName() != nullptr &&
	                               rhi.GetCmdBeginDebugUtilsLabel() != nullptr &&
	                               rhi.GetCmdEndDebugUtilsLabel() != nullptr &&
	                               rhi.GetCmdInsertDebugUtilsLabel() != nullptr;
	return CreateRhiDiagnosticsComposition(
	    std::make_unique<VulkanRenderObjectDiagnostics>(rhi),
	    std::make_unique<VulkanRenderTimingDiagnostics>(rhi),
	    std::make_unique<VulkanRenderMessageDiagnostics>(rhi),
	    nullptr,
	    std::make_unique<VulkanRenderMemoryDiagnostics>(memoryAllocator),
	    supportsGpuEvents);
}
