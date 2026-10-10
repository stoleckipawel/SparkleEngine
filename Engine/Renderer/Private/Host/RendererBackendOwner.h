#pragma once

#include "RHI/Public/Device/RhiDeviceLaunch.h"
#include <memory>

class RenderDeviceServices;
class RendererHost;
class Window;

// Owns the RenderThread-affine device services. Process-facing integration
// lifetime is established before this object receives its immutable bootstrap.
class RendererBackendOwner final
{
public:
	RendererBackendOwner(Window& window, const RhiDeviceLaunch& deviceLaunch);
	~RendererBackendOwner() noexcept;

	RendererBackendOwner(const RendererBackendOwner&) = delete;
	RendererBackendOwner& operator=(const RendererBackendOwner&) = delete;
	RendererBackendOwner(RendererBackendOwner&&) = delete;
	RendererBackendOwner& operator=(RendererBackendOwner&&) = delete;

private:
	friend class RendererHost;

	RenderDeviceServices& GetDeviceServices() noexcept { return *m_deviceServices; }

	std::unique_ptr<RenderDeviceServices> m_deviceServices;
};
