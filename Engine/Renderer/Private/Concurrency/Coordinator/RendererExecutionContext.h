#pragma once

#include "Concurrency/Control/RendererExecutionControl.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include "Viewport/ViewportCaptureCompletion.h"
#include "Renderer/Public/UI/UiTextureHandle.h"

#include <cstdint>
#include <memory>
#include <vector>

class FramePipeline;
class RenderCoordinator;
class RendererHost;
class Window;
struct RhiDeviceLaunch;
struct RendererExecutionConfig;
struct RenderExecutionRequest;
struct ViewportRenderProducts;

class RendererExecutionContext final
{
public:
	RendererExecutionContext(Window& window, const RhiDeviceLaunch& deviceLaunch, const RendererExecutionConfig& executionConfig);
	~RendererExecutionContext() noexcept;

private:
	friend class RenderCoordinator;

	void ExecuteFrame(RenderExecutionRequest request) noexcept;
	void ExecuteControl(RendererExecutionControl control) noexcept;

	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept;
	UiTextureHandle GetViewportPresentationTexture() const noexcept;
	std::vector<ViewportCaptureCompletion> TakeCompletedViewportCaptures();
	std::uint64_t GetShaderGeneration() const noexcept;
	void CompleteShaderReload(const RenderReloadShadersCommand& command) noexcept;
	void CompleteDiagnostics(const RenderDiagnosticsCommand& command);
	void SettleRendererBeforeDestruction() noexcept;

	Threading::OwnerThread m_owner{"RenderCoordinator renderer state"};
	std::unique_ptr<RendererHost> m_rendererHost;
	std::unique_ptr<FramePipeline> m_pipeline;
	bool m_shutdownSettled = false;
#if SPARKLE_WITH_EXTERNAL_CAPTURE
	RhiExternalCapture* m_externalCapture = nullptr;
#endif
};
