#include "PCH.h"
#include "Host/RendererBackendOwner.h"

#include "RHI/Public/Device/RenderDeviceServices.h"

RendererBackendOwner::RendererBackendOwner(Window& window, const RhiDeviceLaunch& deviceLaunch)
{
	m_deviceServices = RenderDeviceServices::Create(window, deviceLaunch);
}

RendererBackendOwner::~RendererBackendOwner() noexcept
{
	m_deviceServices.reset();
}
