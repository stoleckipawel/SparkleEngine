#include "PCH.h"
#include "Host/RendererBackendOwner.h"

#include "RHI/Public/Device/RenderDeviceServices.h"

RendererBackendOwner::RendererBackendOwner(
    Window& window,
    PixelFormat backBufferFormat,
    const RendererBackendConfiguration& configuration) noexcept
{
	m_deviceServices = RenderDeviceServices::Create(
	    window,
	    configuration.BackendApi,
	    backBufferFormat,
	    configuration.D3D12InterposerHooks);
}

RendererBackendOwner::~RendererBackendOwner() noexcept
{
	m_deviceServices.reset();
}
