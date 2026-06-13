#pragma once

#include <cstdint>

class RenderCommandList;
class RenderDiagnostics;
class RenderHardwareInterface;
class RhiImGuiRenderer;

class RenderDeviceBackendServices
{
  public:
	virtual ~RenderDeviceBackendServices() noexcept = default;

	virtual RenderHardwareInterface& GetRenderHardwareInterface() noexcept = 0;
	virtual const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept = 0;
	virtual RhiImGuiRenderer& GetImGuiRenderer() noexcept = 0;
	virtual RenderDiagnostics& GetDiagnostics() noexcept = 0;
	virtual const RenderDiagnostics& GetDiagnostics() const noexcept = 0;
	virtual void Flush() noexcept = 0;
	virtual void ResizeSwapChain() noexcept = 0;
	virtual void BeginFrame() noexcept = 0;
	virtual RenderCommandList& GetCurrentGraphicsCommandList() noexcept = 0;
	virtual RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept = 0;
	virtual void SubmitFrame() noexcept = 0;
	virtual void AdvanceFrameInFlight() noexcept = 0;
	virtual void UpdatePerFrameConstants(std::uint32_t renderViewMode, std::uint32_t viewportWidth, std::uint32_t viewportHeight) noexcept = 0;
	virtual void CloseExecuteAndFlushCurrentFrame() noexcept = 0;
};
