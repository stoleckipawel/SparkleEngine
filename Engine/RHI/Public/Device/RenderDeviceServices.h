#pragma once

#include "../Core/RhiBackendSelection.h"
#include "../Commands/RhiCommandSubmissionService.h"
#include "../RHIAPI.h"
#include "RenderDeviceSettings.h"
#include "RenderHardwareInterface.h"

#include <cstdint>
#include <memory>

class Window;
class RhiImGuiRenderer;

class SPARKLE_RHI_API RenderDeviceServices final : public RhiCommandSubmissionService
{
  public:
	static std::unique_ptr<RenderDeviceServices> Create(Window& window) noexcept;
	static std::unique_ptr<RenderDeviceServices> Create(Window& window, RhiBackendSelection selection) noexcept;
	static std::unique_ptr<RenderDeviceServices> Create(
	    Window& window,
	    const RenderDeviceSettings& settings) noexcept;
	static std::unique_ptr<RenderDeviceServices> Create(
	    Window& window,
	    RhiBackendSelection selection,
	    const RenderDeviceSettings& settings) noexcept;

	~RenderDeviceServices() noexcept;

	RenderDeviceServices(const RenderDeviceServices&) = delete;
	RenderDeviceServices& operator=(const RenderDeviceServices&) = delete;
	RenderDeviceServices(RenderDeviceServices&&) = delete;
	RenderDeviceServices& operator=(RenderDeviceServices&&) = delete;

	const RhiCapabilities& GetCapabilities() const noexcept;
	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	void WaitForIdle() noexcept override;
	void Flush() noexcept override;
	void ResizeSwapChain() noexcept override;
	void BeginFrame() noexcept override;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	void SubmitFrame() noexcept override;
	void AdvanceFrameInFlight() noexcept override;
	void CloseExecuteAndFlushCurrentFrame() noexcept override;

  private:
	RenderDeviceServices() noexcept;

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
