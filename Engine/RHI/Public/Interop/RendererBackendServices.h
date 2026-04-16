#pragma once

#include "../RHIAPI.h"
#include "RenderHardwareInterface.h"

#include <memory>

class Timer;
class Window;

class SPARKLE_RHI_API RendererBackendServices final
{
  public:
	// Keep the public seam focused on orchestration and backend-neutral handles.
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
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept;
	NativeGraphicsCommandListHandle GetCurrentGraphicsCommandListHandle() const noexcept;
	void SubmitFrame() noexcept;
	void AdvanceFrameInFlight() noexcept;
	void UpdatePerFrameConstants(std::uint32_t renderViewMode) noexcept;
	void CloseExecuteAndFlushCurrentFrame() noexcept;

  private:
	RendererBackendServices() noexcept;

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};