#include "Vulkan/Interop/VulkanInteropService.h"

#include "Vulkan/Descriptors/VulkanDescriptorService.h"
#include "Vulkan/VulkanRenderHardwareInterface.h"

VulkanInteropService::VulkanInteropService(VulkanRenderHardwareInterface& owner) noexcept :
    m_owner(&owner)
{
}

RhiNativeDeviceQueueInterop VulkanInteropService::GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept
{
	if (!IsRhiNativeInteropRequestValid(request))
	{
		return {};
	}
	return RhiNativeDeviceQueueInterop{
	    .BackendApi = m_owner != nullptr ? m_owner->GetCapabilities().BackendApi : ERhiBackendApi::Unknown,
	    .Device = m_owner != nullptr ? m_owner->GetDeviceHandle() : NativeGraphicsDeviceHandle{},
	    .GraphicsQueue = m_owner != nullptr ? m_owner->GetGraphicsQueueHandle() : NativeGraphicsQueueHandle{},
	    .Request = request};
}

NativeTextureViewInfo VulkanInteropService::GetNativeTextureViewInfo(
    RhiResourceViewHandle view,
    RhiResourceHandle resource,
    ResourceState state,
    const RhiNativeInteropRequest& request) const noexcept
{
	if (!IsRhiNativeInteropRequestValid(request) || m_owner == nullptr || m_owner->m_descriptorService == nullptr)
	{
		return {};
	}
	return m_owner->m_descriptorService->ResolveNativeTextureViewInfo(view, resource, state);
}
