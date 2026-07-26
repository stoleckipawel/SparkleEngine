#pragma once

#include "Concurrency/Control/RenderControlCommand.h"
#include "Concurrency/FrameQueue/RenderFramePacket.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include "RendererSerialUiCallback.h"

#include <memory>

class FramePipeline;
class RendererSystemRoot;
class RhiImGuiRenderer;
class Window;
struct RendererBackendConfiguration;
struct RendererExecutionConfig;

class RendererExecutionContext final
{
  public:
	RendererExecutionContext(
	    Window& window,
	    const RendererBackendConfiguration& backendConfiguration,
	    const RendererExecutionConfig& executionConfig);
	~RendererExecutionContext() noexcept;

	void ExecuteFrame(RenderFramePacket packet) noexcept;
	void ExecuteControl(const RenderControlPayload& payload) noexcept;
	void StageSerialInput(RenderInputFrame input) noexcept;
	void RenderSerialUiFrame(
	    const TimeInfo& timing,
	    RendererSerialUiCallback composeUi,
	    void* context) noexcept;

	RendererSystemRoot& GetSystems() noexcept;
	const RendererSystemRoot& GetSystems() const noexcept;
	FramePipeline& GetPipeline() noexcept;
	const FramePipeline& GetPipeline() const noexcept;

  private:
	void CompleteDiagnostics(const RenderDiagnosticsCommand& command);
	void SettleRendererBeforeDestruction() noexcept;

	Threading::OwnerThread m_owner{"RenderCoordinator renderer state"};
	std::unique_ptr<RendererSystemRoot> m_systems;
	std::unique_ptr<FramePipeline> m_pipeline;
	bool m_shutdownSettled = false;
};
