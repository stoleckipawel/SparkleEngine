#include "PCH.h"
#include "Concurrency/Coordinator/RendererExecutionContext.h"

#include "Concurrency/FrameQueue/RenderExecutionRequest.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Frame/FramePipeline.h"
#include "Host/RendererBackendConfiguration.h"
#include "Host/RendererHost.h"
#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"
#include "Settings/EngineRenderingSettingsRuntime.h"

RendererExecutionContext::RendererExecutionContext(
    Window& window,
    const RendererBackendConfiguration& backendConfiguration,
    const RendererExecutionConfig& executionConfig)
{
	m_rendererHost = std::make_unique<RendererHost>(window, backendConfiguration);
	m_pipeline = m_rendererHost->CreateFramePipeline(
	    *executionConfig.AssetTaskExecutor,
	    *executionConfig.ApplicationTaskScope,
	    executionConfig.EnableUiRenderPackets);
}

RendererExecutionContext::~RendererExecutionContext() noexcept
{
	m_owner.AssertAccess();
	SettleRendererBeforeDestruction();
	m_pipeline.reset();
	m_rendererHost.reset();
}

void RendererExecutionContext::ExecuteFrame(RenderExecutionRequest request) noexcept
{
	m_owner.AssertAccess();
	m_pipeline->OnRender(std::move(request.Submission), request.Time, request.Ui);
}

void RendererExecutionContext::ExecuteControl(RendererExecutionControl control) noexcept
{
	m_owner.AssertAccess();
	std::visit(
	    [this](auto& command)
	    {
		    using TCommand = std::decay_t<decltype(command)>;
		    if constexpr (std::is_same_v<TCommand, RenderResizeCommand>)
			    m_pipeline->RequestResize(command.Extent, command.Minimized);
		    else if constexpr (std::is_same_v<TCommand, RenderViewportCommand>)
			    m_pipeline->SubmitViewportRenderRequest(std::move(command.Request));
		    else if constexpr (std::is_same_v<TCommand, RenderReloadShadersCommand>)
		    {
			    try
			    {
				    m_rendererHost->ReloadShaders();
				    command.Completion->Complete(std::monostate{});
			    }
			    catch (const Diagnostics::Error& error)
			    {
				    command.Completion->Complete(RenderControlError{error.what()});
			    }
		    }
		    else if constexpr (std::is_same_v<TCommand, RenderDiagnosticsCommand>)
			    CompleteDiagnostics(command);
		    else if constexpr (std::is_same_v<TCommand, RenderCaptureCommand>)
			    (void) m_pipeline->BeginViewportCapture(command.Id, command.Request);
		    else if constexpr (std::is_same_v<TCommand, RenderSettingsChangedCommand>)
			    EngineRenderingSettingsRuntime::Apply(command.Settings);
		    else if constexpr (std::is_same_v<TCommand, RenderShutdownCommand>)
			    SettleRendererBeforeDestruction();
	    },
	    control);
}

const ViewportRenderProducts& RendererExecutionContext::GetViewportRenderProducts() const noexcept
{
	m_owner.AssertAccess();
	return m_pipeline->GetViewportRenderProducts();
}

std::vector<ViewportCaptureCompletion> RendererExecutionContext::TakeCompletedViewportCaptures()
{
	m_owner.AssertAccess();
	return m_pipeline->TakeCompletedViewportCaptures();
}

std::uint64_t RendererExecutionContext::GetShaderGeneration() const noexcept
{
	m_owner.AssertAccess();
	return m_rendererHost->GetShaderGeneration();
}

void RendererExecutionContext::CompleteDiagnostics(const RenderDiagnosticsCommand& command)
{
	switch (command.Kind)
	{
		case RenderDiagnosticsRequestKind::Meshes:
			command.Completion->Complete(m_pipeline->CaptureMeshDiagnostics());
			break;
		case RenderDiagnosticsRequestKind::MeshPreview:
			command.Completion->Complete(m_pipeline->CaptureMeshPreview(command.MeshRuntimeId));
			break;
		case RenderDiagnosticsRequestKind::Textures:
			command.Completion->Complete(m_pipeline->CaptureTextureDiagnostics());
			break;
		case RenderDiagnosticsRequestKind::Memory:
			command.Completion->Complete(m_rendererHost->CaptureMemoryDiagnostics());
			break;
	}
}

void RendererExecutionContext::SettleRendererBeforeDestruction() noexcept
{
	if (m_shutdownSettled || m_rendererHost == nullptr)
	{
		return;
	}

	m_rendererHost->SettleForShutdown();
	m_shutdownSettled = true;
}
