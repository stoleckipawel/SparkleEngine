#pragma once

#include "../Core/RhiBackendSelection.h"
#include "../RHIAPI.h"
#include "RenderHardwareInterface.h"

#include <cstdint>
#include <memory>

class Timer;
class Window;
class RhiImGuiRenderer;

class SPARKLE_RHI_API RenderDeviceServices final
{
  public:
	static std::unique_ptr<RenderDeviceServices> Create(Timer& timer, Window& window) noexcept;
	static std::unique_ptr<RenderDeviceServices> Create(Timer& timer, Window& window, RhiBackendSelection selection) noexcept;

	~RenderDeviceServices() noexcept;

	RenderDeviceServices(const RenderDeviceServices&) = delete;
	RenderDeviceServices& operator=(const RenderDeviceServices&) = delete;
	RenderDeviceServices(RenderDeviceServices&&) = delete;
	RenderDeviceServices& operator=(RenderDeviceServices&&) = delete;

	const RhiCapabilities& GetCapabilities() const noexcept;
	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	RenderDiagnostics& GetDiagnostics() noexcept;
	const RenderDiagnostics& GetDiagnostics() const noexcept;
	void Flush() noexcept;
	void ResizeSwapChain() noexcept;
	void BeginFrame() noexcept;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept;
	void SubmitFrame() noexcept;
	void AdvanceFrameInFlight() noexcept;
	void UpdatePerFrameConstants(std::uint32_t renderViewMode, std::uint32_t viewportWidth, std::uint32_t viewportHeight) noexcept;
	void CloseExecuteAndFlushCurrentFrame() noexcept;

  private:
	RenderDeviceServices() noexcept;

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
