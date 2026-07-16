#include "D3D12/Interop/D3D12InteropService.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/Descriptors/D3D12DescriptorService.h"

D3D12InteropService::D3D12InteropService(D3D12RenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiNativeDeviceQueueInterop D3D12InteropService::GetDeviceQueueInterop(RhiNativeInteropRequest request) const noexcept
{
	if (!IsRhiNativeInteropRequestValid(request))
	{
		return {};
	}
	return RhiNativeDeviceQueueInterop{
	    .BackendApi = m_owner != nullptr ? m_owner->GetBackendApi() : ERhiBackendApi::Unknown,
	    .Device = m_owner != nullptr ? m_owner->GetDeviceHandle() : NativeGraphicsDeviceHandle{},
	    .GraphicsQueue = m_owner != nullptr ? m_owner->GetGraphicsQueueHandle() : NativeGraphicsQueueHandle{},
	    .Request = request};
}

NativeTextureViewInfo D3D12InteropService::GetNativeTextureViewInfo(
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
