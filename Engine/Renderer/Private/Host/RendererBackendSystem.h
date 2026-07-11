#pragma once

#include "RHI/Public/Formats/PixelFormat.h"

#include <memory>

class RenderDeviceServices;
class Window;

// Owns renderer-specific backend bootstrap and external-feature lifetime.
// Callers consume the backend service without knowing which integrations must
// be initialized before device creation.
class RendererBackendSystem final
{
  public:
	explicit RendererBackendSystem(Window& window, PixelFormat backBufferFormat) noexcept;
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
