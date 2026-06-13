#pragma once

#include "Interop/RhiInteropService.h"

class VulkanRenderHardwareInterface;

class VulkanInteropService final : public RhiInteropService
{
  public:
	explicit VulkanInteropService(VulkanRenderHardwareInterface& owner) noexcept;

	RhiNativeDeviceQueueInterop GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept override;
	NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept override;
	NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept override;
	bool UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept override;
	NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept override;

  private:
	VulkanRenderHardwareInterface* m_owner = nullptr;
};
