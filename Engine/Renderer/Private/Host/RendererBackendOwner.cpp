#include "PCH.h"
#include "Host/RendererBackendOwner.h"

#include "RHI/Public/Device/RenderDeviceServices.h"

RendererBackendOwner::RendererBackendOwner(
    Window& window,
    const RendererBackendConfiguration& configuration) noexcept
{
	m_deviceServices = RenderDeviceServices::Create(
	    window,
	    configuration.BackendApi,
	    configuration.InterposerHooks);
}

RendererBackendOwner::~RendererBackendOwner() noexcept
{
	m_deviceServices.reset();
}
