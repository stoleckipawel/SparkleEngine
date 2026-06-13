#pragma once

#include "../Core/RhiBackendSelection.h"
#include "../Commands/RhiCommandSubmissionService.h"
#include "../RHIAPI.h"
#include "RenderHardwareInterface.h"

#include <cstdint>
#include <memory>

class Timer;
class Window;
class RhiImGuiRenderer;

class SPARKLE_RHI_API RenderDeviceServices final : public RhiCommandSubmissionService
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
	void WaitForIdle() noexcept override;
	void Flush() noexcept override;
	void ResizeSwapChain() noexcept override;
	void BeginFrame() noexcept override;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	void SubmitFrame() noexcept override;
	void AdvanceFrameInFlight() noexcept override;
	void UpdatePerFrameConstants(std::uint32_t renderViewMode, std::uint32_t viewportWidth, std::uint32_t viewportHeight) noexcept;
	void CloseExecuteAndFlushCurrentFrame() noexcept override;

  private:
	RenderDeviceServices() noexcept;

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
