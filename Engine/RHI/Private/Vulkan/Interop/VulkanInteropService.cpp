#include "Vulkan/Interop/VulkanInteropService.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

VulkanInteropService::VulkanInteropService(VulkanRenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiNativeDeviceQueueInterop VulkanInteropService::GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept
{
	return RhiNativeDeviceQueueInterop{
	    .BackendApi = m_owner != nullptr ? m_owner->GetBackendApi() : ERhiBackendApi::Unknown,
	    .Device = m_owner != nullptr ? m_owner->GetDeviceHandle() : NativeGraphicsDeviceHandle{},
	    .GraphicsQueue = m_owner != nullptr ? m_owner->GetGraphicsQueueHandle() : NativeGraphicsQueueHandle{},
	    .Request = request};
}

bool VulkanInteropService::UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept
{
	return m_owner != nullptr && m_owner->UpgradePresentationInterface(callback, userData);
}

NativeTextureViewInfo VulkanInteropService::GetNativeTextureViewInfo(
    RhiResourceViewHandle view,
    ResourceState state) const noexcept
{
	return m_owner != nullptr ? m_owner->GetNativeTextureViewInfo(view, state) : NativeTextureViewInfo{};
}
