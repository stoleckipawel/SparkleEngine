#pragma once

#include "Host/RendererBackendConfiguration.h"
#include <memory>

class RenderDeviceServices;
class Window;

// Owns the RenderThread-affine device services. Process-facing integration
// lifetime is established before this object receives its immutable bootstrap.
class RendererBackendOwner final
{
  public:
	RendererBackendOwner(
	    Window& window,
	    const RendererBackendConfiguration& configuration) noexcept;
	~RendererBackendOwner() noexcept;

	RendererBackendOwner(const RendererBackendOwner&) = delete;
	RendererBackendOwner& operator=(const RendererBackendOwner&) = delete;
	RendererBackendOwner(RendererBackendOwner&&) = delete;
	RendererBackendOwner& operator=(RendererBackendOwner&&) = delete;

	RenderDeviceServices& GetDeviceServices() noexcept { return *m_deviceServices; }
	const RenderDeviceServices& GetDeviceServices() const noexcept { return *m_deviceServices; }

  private:
	std::unique_ptr<RenderDeviceServices> m_deviceServices;
};
