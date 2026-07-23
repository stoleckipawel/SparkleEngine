#pragma once

#include "Host/RendererBackendConfiguration.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <memory>

class RenderDeviceServices;
class Window;

// Owns the RenderThread-affine device services. Process-facing integration
// lifetime is established before this object receives its immutable bootstrap.
class RendererBackendSystem final
{
  public:
	RendererBackendSystem(
	    Window& window,
	    PixelFormat backBufferFormat,
	    const RendererBackendConfiguration& configuration) noexcept;
	~RendererBackendSystem() noexcept;

	RendererBackendSystem(const RendererBackendSystem&) = delete;
	RendererBackendSystem& operator=(const RendererBackendSystem&) = delete;
	RendererBackendSystem(RendererBackendSystem&&) = delete;
	RendererBackendSystem& operator=(RendererBackendSystem&&) = delete;

	RenderDeviceServices& GetServices() noexcept { return *m_services; }
	const RenderDeviceServices& GetServices() const noexcept { return *m_services; }

  private:
	std::unique_ptr<RenderDeviceServices> m_services;
};
