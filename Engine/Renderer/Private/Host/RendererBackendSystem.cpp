#include "PCH.h"
#include "Host/RendererBackendSystem.h"

#include "RHI/Public/Device/RenderDeviceServices.h"

RendererBackendSystem::RendererBackendSystem(
    Window& window,
    PixelFormat backBufferFormat,
    const RendererBackendConfiguration& configuration) noexcept
{
	m_services = RenderDeviceServices::Create(
	    window,
	    configuration.BackendApi,
	    backBufferFormat,
	    configuration.ExternalFeatureHooks);
}

RendererBackendSystem::~RendererBackendSystem() noexcept
{
	m_services.reset();
}
