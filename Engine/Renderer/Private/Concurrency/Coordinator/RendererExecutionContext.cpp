#include "PCH.h"
#include "Concurrency/Coordinator/RendererExecutionContext.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Frame/FramePipeline.h"
#include "Host/RendererBackendConfiguration.h"
#include "Host/RendererHost.h"
#include "Providers/RendererImageProviderStack.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"

RendererExecutionContext::RendererExecutionContext(
    Window& window,
    const RendererBackendConfiguration& backendConfiguration,
    const RendererExecutionConfig& executionConfig)
{
	m_rendererHost = std::make_unique<RendererHost>(
	    window,
	    backendConfiguration,
	    *executionConfig.AssetTaskExecutor,
	    *executionConfig.ApplicationTaskScope);
	m_pipeline = std::make_unique<FramePipeline>(
	    m_rendererHost->GetWindow(),
	    m_rendererHost->GetDeviceServices(),
	    m_rendererHost->GetRenderPassRuntimeCache(),
	    m_rendererHost->GetMemoryMonitor(),
	    m_rendererHost->GetGpuMeshCache(),
	    m_rendererHost->GetTextureCache(),
	    m_rendererHost->GetRenderScenePreparation(),
	    m_rendererHost->GetRenderViewBuilder(),
	    m_rendererHost->GetRenderViewPreparation(),
	    m_rendererHost->GetRenderViewState(),
	    m_rendererHost->GetRenderScene(),
	    m_rendererHost->GetImageProviders(),
	    m_rendererHost->GetTaskExecutor(),
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

void RendererExecutionContext::ExecuteControl(RenderControlPayload payload) noexcept
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
				    m_rendererHost->GetRenderPassRuntimeCache().ReloadCookedShaders();
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
		    else if constexpr (std::is_same_v<TCommand, RenderRefreshProvidersCommand>)
			    m_rendererHost->GetImageProviders().Refresh();
		    else if constexpr (std::is_same_v<TCommand, RenderSettingsChangedCommand>)
			    ApplyEngineRenderingSettingsStateToCVars(command.Settings);
		    else if constexpr (std::is_same_v<TCommand, RenderShutdownCommand>)
			    SettleRendererBeforeDestruction();
		    else
		    {
			    // Frame-ready commands are consumed by RenderCoordinator.
		    }
	    },
	    payload);
}

RendererHost& RendererExecutionContext::GetRendererHost() noexcept
{
	m_owner.AssertAccess();
	return *m_rendererHost;
}

const RendererHost& RendererExecutionContext::GetRendererHost() const noexcept
{
	m_owner.AssertAccess();
	return *m_rendererHost;
}

FramePipeline& RendererExecutionContext::GetPipeline() noexcept
{
	m_owner.AssertAccess();
	return *m_pipeline;
}

const FramePipeline& RendererExecutionContext::GetPipeline() const noexcept
{
	m_owner.AssertAccess();
	return *m_pipeline;
}

void RendererExecutionContext::CompleteDiagnostics(const RenderDiagnosticsCommand& command)
{
	switch (command.Kind)
	{
		case RenderDiagnosticsRequestKind::Meshes:
			command.Completion->Complete(
			    MeshDiagnosticsCollector::Capture(m_rendererHost->GetRenderScene(), &m_rendererHost->GetGpuMeshCache()));
			break;
		case RenderDiagnosticsRequestKind::MeshPreview:
			command.Completion->Complete(MeshDiagnosticsCollector::CapturePreview(m_rendererHost->GetRenderScene(), command.MeshRuntimeId));
			break;
		case RenderDiagnosticsRequestKind::Textures:
			command.Completion->Complete(m_pipeline->CaptureTextureDiagnostics());
			break;
		case RenderDiagnosticsRequestKind::Memory:
			command.Completion->Complete(m_rendererHost->GetMemoryMonitor().GetLatestSnapshot());
			break;
	}
}

void RendererExecutionContext::SettleRendererBeforeDestruction() noexcept
{
	if (m_shutdownSettled || m_rendererHost == nullptr)
	{
		return;
	}

	m_rendererHost->GetDeviceServices().SettleForShutdown();
	m_shutdownSettled = true;
}
