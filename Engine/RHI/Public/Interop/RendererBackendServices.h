#pragma once

#include "RHIAPI.h"
#include "Interop/RenderHardwareInterface.h"

#include <memory>

class Timer;
class Window;

namespace Rhi::Internal
{
	struct RendererBackendServicesAccess;
}

class SPARKLE_RHI_API RendererBackendServices final
{
  public:
	// Keep the public seam focused on orchestration and backend-neutral handles.
	// Renderer-private backend detail access lives behind Interop/Internal helpers.
	static std::unique_ptr<RendererBackendServices> Create(Timer& timer, Window& window) noexcept;

	~RendererBackendServices() noexcept;

	RendererBackendServices(const RendererBackendServices&) = delete;
	RendererBackendServices& operator=(const RendererBackendServices&) = delete;
	RendererBackendServices(RendererBackendServices&&) = delete;
	RendererBackendServices& operator=(RendererBackendServices&&) = delete;

	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	void Flush() noexcept;
	void ResizeSwapChain() noexcept;
	void BeginFrame() noexcept;
	NativeGraphicsCommandListHandle GetCurrentGraphicsCommandListHandle() const noexcept;
	void SubmitFrame() noexcept;
	void AdvanceFrameInFlight() noexcept;
	void UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept;
	void CloseExecuteAndFlushCurrentFrame() noexcept;

  private:
	RendererBackendServices() noexcept;

	friend struct Rhi::Internal::RendererBackendServicesAccess;

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};