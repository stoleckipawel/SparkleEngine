#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Diagnostics/VulkanRenderDiagnostics.h"

#include "Device/RenderHardwareInterface.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"

#include "Core/Public/Strings/StringUtils.h"

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
		SetDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<std::uint64_t>(commandList.GetNativeHandle().Value), debugName);
	}

	void SetDebugName(NativeResourceHandle resource, std::wstring_view debugName) noexcept override
	{
		SetDebugName(VK_OBJECT_TYPE_UNKNOWN, reinterpret_cast<std::uint64_t>(resource.Value), debugName);
	}

	void SetDebugName(RhiOwnedMemoryBlockHandle, std::wstring_view) noexcept override {}

	void SetDebugName(RhiOwnedResourceHandle resource, std::wstring_view debugName) noexcept override
	{
		SetDebugName(VK_OBJECT_TYPE_UNKNOWN, reinterpret_cast<std::uint64_t>(resource.Value), debugName);
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

	bool SupportsDebugMessages() const noexcept override { return true; }

	bool TryPopMessage(RhiDiagnosticMessage& outMessage) noexcept override { return m_rhi.TryPopDiagnosticMessage(outMessage); }

	void ClearMessages() noexcept override { m_rhi.ClearDiagnosticMessages(); }

  private:
	VulkanRhi& m_rhi;
};

class VulkanRenderFailureDiagnostics final : public RenderFailureDiagnostics
{
  public:
	bool SupportsLiveObjectReports() const noexcept override { return false; }

	bool SupportsCrashDiagnostics() const noexcept override { return false; }

	void ReportLiveObjects() noexcept override {}

	void CollectCrashDiagnostics() noexcept override {}
};

class VulkanRenderDiagnostics final : public RenderDiagnostics
{
  public:
	explicit VulkanRenderDiagnostics(VulkanRhi& rhi) noexcept : m_objectDiagnostics(rhi), m_messageDiagnostics(rhi) {}

	RhiDiagnosticsCapabilities GetCapabilities() const noexcept override
	{
		return RhiDiagnosticsCapabilities{
		    .SupportsObjectNames = m_objectDiagnostics.SupportsObjectNames(),
		    .SupportsGpuEvents = false,
		    .SupportsTimestampQueries = false,
		    .SupportsDebugMessages = true,
		    .SupportsLiveObjectReports = false,
		    .SupportsCrashDiagnostics = false,
		    .SupportsMemoryDiagnostics = false,
		    .SupportsMemoryBudgetQueries = false,
		    .SupportsMemoryJsonDump = false};
	}

	RenderObjectDiagnostics& GetObjectDiagnostics() noexcept override { return m_objectDiagnostics; }

	const RenderObjectDiagnostics& GetObjectDiagnostics() const noexcept override { return m_objectDiagnostics; }

	RenderTimingDiagnostics* GetTimingDiagnostics() noexcept override { return nullptr; }

	const RenderTimingDiagnostics* GetTimingDiagnostics() const noexcept override { return nullptr; }

	RenderMessageDiagnostics* GetMessageDiagnostics() noexcept override { return &m_messageDiagnostics; }

	const RenderMessageDiagnostics* GetMessageDiagnostics() const noexcept override { return &m_messageDiagnostics; }

	RenderFailureDiagnostics* GetFailureDiagnostics() noexcept override { return nullptr; }

	const RenderFailureDiagnostics* GetFailureDiagnostics() const noexcept override { return nullptr; }

	RenderMemoryDiagnostics* GetMemoryDiagnostics() noexcept override { return nullptr; }

	const RenderMemoryDiagnostics* GetMemoryDiagnostics() const noexcept override { return nullptr; }

  private:
	VulkanRenderObjectDiagnostics m_objectDiagnostics;
	VulkanRenderMessageDiagnostics m_messageDiagnostics;
	VulkanRenderFailureDiagnostics m_failureDiagnostics;
};

std::unique_ptr<RenderDiagnostics> CreateVulkanRenderDiagnostics(VulkanRhi& rhi)
{
	return std::make_unique<VulkanRenderDiagnostics>(rhi);
}