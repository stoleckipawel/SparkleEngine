#pragma once

#include "Core/Public/Threading/ThreadOwnership.h"
#include "Host/RendererBackendConfiguration.h"

// Owns process-facing renderer integration initialization on the application
// thread. RenderCoordinator receives only the immutable device bootstrap value.
class RendererExternalRuntime final
{
  public:
	RendererExternalRuntime() noexcept;
	~RendererExternalRuntime() noexcept;

	RendererExternalRuntime(const RendererExternalRuntime&) = delete;
	RendererExternalRuntime& operator=(const RendererExternalRuntime&) = delete;

	const RendererBackendConfiguration& GetBackendConfiguration() const noexcept;

  private:
	Threading::OwnerThread m_owner{"Renderer external runtime"};
	RendererBackendConfiguration m_backendConfiguration;
};
