#include "PCH.h"
#include "Host/RendererBackendSystem.h"

#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Streamline/StreamlineRuntimeSupport.h"

RendererBackendSystem::RendererBackendSystem(Window& window, PixelFormat backBufferFormat) noexcept
{
	const ERhiBackendApi backendApi = ResolveDefaultRhiBackendApi();
	(void) InitializeSharedStreamlineRuntime(backendApi);
	m_services = RenderDeviceServices::Create(
	    window,
	    backendApi,
	    backBufferFormat,
	    GetSharedStreamlineRhiHooks());
}

RendererBackendSystem::~RendererBackendSystem() noexcept
{
	// RHI objects may still refer to upgraded external interfaces. Destroy them
	// before shutting down the integration that owns those interfaces.
	m_services.reset();
	ShutdownSharedStreamlineRuntime();
}
