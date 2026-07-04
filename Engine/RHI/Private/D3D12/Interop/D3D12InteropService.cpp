#include "D3D12/Interop/D3D12InteropService.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

D3D12InteropService::D3D12InteropService(D3D12RenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiNativeDeviceQueueInterop D3D12InteropService::GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept
{
	return RhiNativeDeviceQueueInterop{
	    .BackendApi = m_owner != nullptr ? m_owner->GetBackendApi() : ERhiBackendApi::Unknown,
	    .Device = m_owner != nullptr ? m_owner->GetDeviceHandle() : NativeGraphicsDeviceHandle{},
	    .GraphicsQueue = m_owner != nullptr ? m_owner->GetGraphicsQueueHandle() : NativeGraphicsQueueHandle{},
	    .Request = request};
}

bool D3D12InteropService::UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept
{
	return m_owner != nullptr && m_owner->UpgradePresentationInterface(callback, userData);
}
