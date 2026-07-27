#include "PCH.h"
#include "Concurrency/Coordinator/RendererExecutionContext.h"

#include "FramePipeline/FramePipeline.h"
#include "Host/RendererBackendConfiguration.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"

RendererExecutionContext::RendererExecutionContext(
    Window& window,
    const RendererBackendConfiguration& backendConfiguration,
    const RendererExecutionConfig& executionConfig)
{
	m_systems = std::make_unique<RendererSystemRoot>(
	    window,
	    backendConfiguration,
	    *executionConfig.AssetTaskExecutor,
	    *executionConfig.ApplicationTaskScope);
	m_pipeline = std::make_unique<FramePipeline>(
	    *m_systems,
	    executionConfig.EnableUiRenderPackets);
	m_systems->PostLoad();
}

RendererExecutionContext::~RendererExecutionContext() noexcept
{
	m_owner.AssertAccess();
	SettleRendererBeforeDestruction();
	m_pipeline.reset();
	m_systems.reset();
}

void RendererExecutionContext::ExecuteFrame(RenderFramePacket packet) noexcept
{
	m_owner.AssertAccess();
	m_pipeline->SubmitRenderInput(std::move(packet.Input));
	m_pipeline->OnRender(packet.Timing, packet.Ui);
}

void RendererExecutionContext::ExecuteControl(const RenderControlPayload& payload) noexcept
{
	m_owner.AssertAccess();
	std::visit(
	    [this](const auto& command)
	    {
		    using TCommand = std::decay_t<decltype(command)>;
		    if constexpr (std::is_same_v<TCommand, RenderResizeCommand>)
			    m_pipeline->RequestResize(command.Extent, command.Minimized);
		    else if constexpr (std::is_same_v<TCommand, RenderViewportCommand>)
			    m_pipeline->SubmitViewportRenderRequest(command.Request);
		    else if constexpr (std::is_same_v<TCommand, RenderReloadShadersCommand>)
			    command.Completion->Complete(m_systems->ReloadCookedShaders());
		    else if constexpr (std::is_same_v<TCommand, RenderDiagnosticsCommand>)
			    CompleteDiagnostics(command);
		    else if constexpr (std::is_same_v<TCommand, RenderCaptureCommand>)
			    (void) m_pipeline->BeginViewportCapture(command.Id, command.Request);
		    else if constexpr (std::is_same_v<TCommand, RenderRefreshProvidersCommand>)
			    m_systems->RefreshImageProviders();
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

RendererSystemRoot& RendererExecutionContext::GetSystems() noexcept
{
	m_owner.AssertAccess();
	return *m_systems;
}

const RendererSystemRoot& RendererExecutionContext::GetSystems() const noexcept
{
	m_owner.AssertAccess();
	return *m_systems;
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
			command.Completion->Complete(m_systems->CaptureMeshDiagnostics());
			break;
		case RenderDiagnosticsRequestKind::MeshPreview:
			command.Completion->Complete(m_systems->CaptureMeshPreview(command.MeshRuntimeId));
			break;
		case RenderDiagnosticsRequestKind::Textures:
			command.Completion->Complete(m_pipeline->CaptureTextureDiagnostics());
			break;
		case RenderDiagnosticsRequestKind::Memory:
			command.Completion->Complete(m_systems->CaptureMemoryDiagnostics());
			break;
	}
}

void RendererExecutionContext::SettleRendererBeforeDestruction() noexcept
{
	if (m_shutdownSettled || m_systems == nullptr)
	{
		return;
	}

	m_systems->GetBackend().SettleForShutdown();
	m_shutdownSettled = true;
}
