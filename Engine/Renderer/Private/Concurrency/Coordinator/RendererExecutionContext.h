#pragma once

#include "Concurrency/Control/RendererExecutionControl.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include "Viewport/ViewportCaptureCompletion.h"

#include <cstdint>
#include <memory>
#include <vector>

class FramePipeline;
class RenderCoordinator;
class RendererHost;
class Window;
struct RendererBackendConfiguration;
struct RendererExecutionConfig;
struct RenderExecutionRequest;
struct ViewportRenderProducts;

class RendererExecutionContext final
{
public:
	RendererExecutionContext(
	    Window& window,
	    const RendererBackendConfiguration& backendConfiguration,
	    const RendererExecutionConfig& executionConfig);
	~RendererExecutionContext() noexcept;

private:
	friend class RenderCoordinator;

	void ExecuteFrame(RenderExecutionRequest request) noexcept;
	void ExecuteControl(RendererExecutionControl control) noexcept;

	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept;
	std::vector<ViewportCaptureCompletion> TakeCompletedViewportCaptures();
	std::uint64_t GetShaderGeneration() const noexcept;
	void CompleteDiagnostics(const RenderDiagnosticsCommand& command);
	void SettleRendererBeforeDestruction() noexcept;

	Threading::OwnerThread m_owner{"RenderCoordinator renderer state"};
	std::unique_ptr<RendererHost> m_rendererHost;
	std::unique_ptr<FramePipeline> m_pipeline;
	bool m_shutdownSettled = false;
};
