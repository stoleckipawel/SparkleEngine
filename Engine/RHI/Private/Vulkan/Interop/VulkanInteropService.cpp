#include "Vulkan/Interop/VulkanInteropService.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

VulkanInteropService::VulkanInteropService(VulkanRenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiNativeDeviceQueueInterop VulkanInteropService::GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept
{
	if (!IsRhiNativeInteropRequestValid(request))
	{
		return {};
	}
	return RhiNativeDeviceQueueInterop{
	    .BackendApi = m_owner != nullptr ? m_owner->GetBackendApi() : ERhiBackendApi::Unknown,
	    .Device = m_owner != nullptr ? m_owner->GetDeviceHandle() : NativeGraphicsDeviceHandle{},
	    .GraphicsQueue = m_owner != nullptr ? m_owner->GetGraphicsQueueHandle() : NativeGraphicsQueueHandle{},
	    .Vulkan =
	        RhiNativeVulkanDeviceQueueInterop{
	            .Instance = m_owner != nullptr ? m_owner->GetVulkanInstance() : nullptr,
	            .PhysicalDevice = m_owner != nullptr ? m_owner->GetVulkanPhysicalDevice() : nullptr,
	            .Device = m_owner != nullptr ? m_owner->GetVulkanDevice() : nullptr,
	            .GraphicsQueue = m_owner != nullptr ? m_owner->GetVulkanGraphicsQueue() : nullptr,
	            .GraphicsQueueFamilyIndex = m_owner != nullptr ? m_owner->GetVulkanGraphicsQueueFamilyIndex() : UINT32_MAX},
	    .Request = request};
}
