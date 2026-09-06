#include "PCH.h"
#include "Concurrency/Coordinator/RenderCoordinator.h"

#include "Concurrency/Coordinator/RendererExecutionContext.h"

static const auto g_renderCoordinatorLogger = Logging::GetOrCreateLogger("Renderer.Coordinator");

void RenderCoordinator::Initialize()
{
	if (m_config.IsThreaded())
	{
		InitializeThreaded();
	}
	else
	{
		InitializeSerial();
	}
}

void RenderCoordinator::InitializeSerial()
{
	m_context = std::make_unique<RendererExecutionContext>(*m_window, m_backendConfiguration, m_config);
	SubmitResize();
	PublishReadState();
}

void RenderCoordinator::InitializeThreaded()
{
	m_frameQueue = std::make_unique<RenderFrameQueue>(m_config.GetFrameQueueCapacity());
	m_controlQueue = std::make_unique<RenderControlCommandQueue>(RenderControlCapacity);
	StartRenderThread();
	if (!WaitForRenderThreadStart())
	{
		HandleRenderThreadStartFailure();
		return;
	}

	SubmitResize();
}

void RenderCoordinator::StartRenderThread()
{
	m_renderThread = std::thread([this] { RenderThreadMain(); });
}

bool RenderCoordinator::WaitForRenderThreadStart()
{
	std::unique_lock lock(m_startMutex);
	m_startedCondition.wait(lock, [this] { return m_started; });

	return m_startSucceeded;
}

void RenderCoordinator::HandleRenderThreadStartFailure()
{
	if (m_renderThread.joinable())
	{
		m_renderThread.join();
	}

	Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "RenderThread failed to create its renderer execution context.");
}

void RenderCoordinator::RenderThreadMain()
{
	Threading::SetCurrentThreadRole("Sparkle.RenderThread");
	try
	{
		m_context = std::make_unique<RendererExecutionContext>(*m_window, m_backendConfiguration, m_config);
		{
			std::lock_guard lock(m_startMutex);
			m_startSucceeded = true;
			m_started = true;
		}
		m_startedCondition.notify_one();
		PublishReadState();

		while (std::optional<RenderControlCommand> command = m_controlQueue->WaitPop())
		{
			const bool shutdown = std::holds_alternative<RenderShutdownCommand>(command->Payload);
			ProcessThreadedCommand(std::move(*command));
			if (shutdown)
			{
				break;
			}
		}
	}
	catch (const std::exception& exception)
	{
		SPDLOG_ERROR("RenderThread terminated after an exception: {}", exception.what());
	}
	catch (...)
	{
		SPDLOG_ERROR("RenderThread terminated after an unknown exception.");
	}

	{
		std::lock_guard lock(m_startMutex);
		m_started = true;
	}
	m_startedCondition.notify_one();
	SettleAbandonedWork();
	m_context.reset();
}

void RenderCoordinator::ProcessThreadedCommand(RenderControlCommand command)
{
	if (command.SequenceNumber <= m_lastConsumedControlSequence)
	{
		Diagnostics::Fatal(
		    g_renderCoordinatorLogger,
		    __FILE__,
		    __LINE__,
		    "Render-control command sequence was consumed more than once or out of order.");
	}
	m_lastConsumedControlSequence = command.SequenceNumber;
	if (const auto* frame = std::get_if<RenderFrameReadyCommand>(&command.Payload))
	{
		ExecuteThreadedFrame(frame->Ticket);
	}
	else
	{
		m_context->ExecuteControl(std::move(command.Payload));
		PublishReadState();
	}
}

void RenderCoordinator::ExecuteThreadedFrame(RenderFrameQueueTicket ticket)
{
	RenderExecutionRequest request;
	if (!m_frameQueue->Consume(ticket, request))
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render frame queue rejected a queued frame ticket.");
	}
	m_context->ExecuteFrame(std::move(request));
	PublishReadState();
	if (!m_frameQueue->Retire(ticket))
	{
		Diagnostics::Fatal(
		    g_renderCoordinatorLogger,
		    __FILE__,
		    __LINE__,
		    "Render frame queue rejected retirement for the frame being rendered.");
	}
}

void RenderCoordinator::SettleAbandonedWork() noexcept
{
	m_controlQueue->Close();
	for (RenderControlCommand& command : m_controlQueue->Drain())
	{
		if (auto* reloadShaders = std::get_if<RenderReloadShadersCommand>(&command.Payload))
		{
			reloadShaders->Completion->Cancel();
		}
		else if (auto* diagnostics = std::get_if<RenderDiagnosticsCommand>(&command.Payload))
		{
			diagnostics->Completion->Cancel();
		}
	}
	m_frameQueue->SettleAll();
}
