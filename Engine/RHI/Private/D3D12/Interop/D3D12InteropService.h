#pragma once

#include "Interop/RhiInteropService.h"

class D3D12RenderHardwareInterface;

class D3D12InteropService final : public RhiInteropService
{
  public:
	explicit D3D12InteropService(D3D12RenderHardwareInterface& owner) noexcept;

	RhiNativeDeviceQueueInterop GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept override;
	bool UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept override;
	NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept override;

  private:
	D3D12RenderHardwareInterface* m_owner = nullptr;
};
