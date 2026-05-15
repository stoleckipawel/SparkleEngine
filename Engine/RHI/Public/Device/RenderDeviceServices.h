#pragma once

#include "../RHIAPI.h"
#include "RenderHardwareInterface.h"

#include <cstdint>
#include <memory>

class Timer;
class Window;

class SPARKLE_RHI_API RenderDeviceServices final
{
  public:
	static std::unique_ptr<RenderDeviceServices> Create(Timer& timer, Window& window) noexcept;

	~RenderDeviceServices() noexcept;

	RenderDeviceServices(const RenderDeviceServices&) = delete;
	RenderDeviceServices& operator=(const RenderDeviceServices&) = delete;
	RenderDeviceServices(RenderDeviceServices&&) = delete;
	RenderDeviceServices& operator=(RenderDeviceServices&&) = delete;

	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RenderDiagnostics& GetDiagnostics() noexcept;
	const RenderDiagnostics& GetDiagnostics() const noexcept;
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
	RenderDeviceServices() noexcept;

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
