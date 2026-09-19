#include "PCH.h"
#include "Concurrency/Coordinator/RenderCoordinator.h"

#include "Concurrency/Coordinator/RendererExecutionContext.h"
#include "Time/Timer.h"
#include "Window/Window.h"

static const auto g_renderCoordinatorLogger = Logging::GetOrCreateLogger("Renderer.Coordinator");

RenderCoordinator::RenderCoordinator(
    Timer& timer,
    Window& window,
    RendererExecutionConfig config,
    RendererBackendConfiguration backendConfiguration) :
    m_timer(&timer),
    m_window(&window),
    m_config(config),
    m_backendConfiguration(backendConfiguration)
{
	if (!m_config.HasAssetTaskRuntime())
	{
		Diagnostics::Fatal(
		    g_renderCoordinatorLogger,
		    __FILE__,
		    __LINE__,
		    "RendererExecutionConfig has no asset-task executor or parent scope.");
	}
	Initialize();
	m_resizeHandle = ScopedEventHandle(window.OnResized, window.OnResized.Add([this] { SubmitResize(); }));
}

RenderCoordinator::~RenderCoordinator() noexcept
{
	m_producerOwner.AssertAccess();
	m_resizeHandle = {};
	if (!m_config.IsThreaded())
	{
		m_context.reset();
		return;
	}

	SubmitThreadCommand(RendererExecutionControl{RenderShutdownCommand{}});
	m_frameQueue->Close();
	m_threadCommandQueue->Close();
	if (m_renderThread.joinable())
	{
		m_renderThread.join();
	}
}

void RenderCoordinator::StageFrameSubmission(RenderFrameSubmission submission)
{
	m_producerOwner.AssertAccess();
	if (m_pendingSubmission)
	{
		RenderFrame();
	}

	m_pendingSubmission = std::move(submission);
}

void RenderCoordinator::StageUiRenderPacket(UiRenderPacket packet)
{
	m_producerOwner.AssertAccess();
	m_pendingUi = std::move(packet);
}

void RenderCoordinator::SubmitRenderingSettings(EngineRenderingSettingsState settings)
{
	m_producerOwner.AssertAccess();
	DispatchControl(RenderSettingsChangedCommand{std::move(settings)});
}

void RenderCoordinator::SubmitViewportRequest(ViewportRenderRequest request)
{
	m_producerOwner.AssertAccess();
	DispatchControl(RenderViewportCommand{std::move(request)});
}

void RenderCoordinator::RenderFrame()
{
	m_producerOwner.AssertAccess();
	if (!m_pendingSubmission)
	{
		return;
	}

	if (m_config.IsThreaded())
	{
		SubmitThreadedFrame();
	}
	else
	{
		ExecuteSerialFrame();
	}
}

RenderExecutionRequest RenderCoordinator::TakePendingExecutionRequest()
{
	const TimeInfo timing = m_timer->GetTimeInfo();
	if (timing.frameIndex != m_pendingSubmission->FrameId)
	{
		Diagnostics::Fatal(
		    g_renderCoordinatorLogger,
		    __FILE__,
		    __LINE__,
		    "Render frame submission identity does not match the application timer frame.");
	}

	RenderExecutionRequest request{
	    .Submission = std::move(*m_pendingSubmission),
	    .Time =
	        {
	            .UnscaledTime = timing.unscaledTime,
	            .ScaledTime = timing.scaledTime,
	            .UnscaledDelta = timing.unscaledDelta,
	            .ScaledDelta = timing.scaledDelta,
	        },
	    .Ui = m_pendingUi ? std::move(*m_pendingUi) : UiRenderPacket{}};

	m_pendingSubmission.reset();
	m_pendingUi.reset();
	return request;
}

void RenderCoordinator::ExecuteSerialFrame()
{
	GetSerialContext().ExecuteFrame(TakePendingExecutionRequest());
	PublishReadState();
}

void RenderCoordinator::SubmitThreadedFrame()
{
	const std::optional<RenderFrameQueueTicket> ticket = m_frameQueue->Acquire();
	if (!ticket)
	{
		Diagnostics::Fatal(
		    g_renderCoordinatorLogger,
		    __FILE__,
		    __LINE__,
		    "Render frame queue closed while the producer was acquiring a slot.");
	}

	if (!m_frameQueue->Publish(*ticket, TakePendingExecutionRequest()))
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render frame queue rejected its producer-owned writing ticket.");
	}

	SubmitThreadCommand(RenderFrameReadyCommand{*ticket});

	if (m_config.RenderPipelineDepth == 0u)
	{
		if (!m_frameQueue->WaitUntilReusable(*ticket))
		{
			Diagnostics::Fatal(
			    g_renderCoordinatorLogger,
			    __FILE__,
			    __LINE__,
			    "Render frame queue closed before synchronous frame reuse completed.");
		}
	}
}

ViewportRenderProducts RenderCoordinator::GetViewportRenderProducts() const
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		return GetSerialContext().GetViewportRenderProducts();
	}

	std::lock_guard lock(m_readStateMutex);
	return m_publishedViewportProducts;
}

UiTextureHandle RenderCoordinator::GetViewportPresentationTexture() const
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		return GetSerialContext().GetViewportPresentationTexture();
	}

	std::lock_guard lock(m_readStateMutex);
	return m_publishedViewportTexture;
}

void RenderCoordinator::SubmitResize()
{
	const RenderResizeCommand command{{m_window->GetWidth(), m_window->GetHeight()}, m_window->IsMinimized()};
	if (m_context)
	{
		DispatchControl(command);
	}
}

RendererExecutionContext& RenderCoordinator::GetSerialContext()
{
	m_producerOwner.AssertAccess();
	if (m_config.IsThreaded())
	{
		Diagnostics::Fatal(
		    g_renderCoordinatorLogger,
		    __FILE__,
		    __LINE__,
		    "Serial renderer capability requested while RenderCoordinator owns a RenderThread.");
	}
	return *m_context;
}

const RendererExecutionContext& RenderCoordinator::GetSerialContext() const
{
	m_producerOwner.AssertAccess();
	return const_cast<RenderCoordinator*>(this)->GetSerialContext();
}
