#include "PCH.h"
#include "Concurrency/Coordinator/RenderCoordinator.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Concurrency/Coordinator/RendererExecutionContext.h"
#include "FramePipeline/FramePipeline.h"
#include "Host/RendererHost.h"
#include "Time/Timer.h"
#include "Window/Window.h"

#include <limits>

static const auto g_renderCoordinatorLogger = Logging::GetOrCreateLogger("Renderer.Coordinator");

template <typename TResult> TResult RenderCoordinator::ExtractControlResult(RenderControlResult result)
{
	if (RenderControlError* error = std::get_if<RenderControlError>(&result))
	{
		throw Diagnostics::Error(std::move(error->Message));
	}
	if (TResult* value = std::get_if<TResult>(&result))
	{
		return std::move(*value);
	}
	Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render control returned an incompatible payload.");
}

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
		    "RendererExecutionConfig has no application TaskExecutor or root TaskScope.");
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

	SubmitControl(RenderShutdownCommand{});
	m_frameQueue->Close();
	m_controlQueue->Close();
	if (m_renderThread.joinable())
	{
		m_renderThread.join();
	}
}

void RenderCoordinator::StageRenderInput(RenderInputFrame input)
{
	m_producerOwner.AssertAccess();
	if (m_pendingInput)
	{
		RenderFrame();
	}

	m_pendingInput = std::move(input);
}

void RenderCoordinator::StageUiRenderPacket(UiRenderPacket packet)
{
	m_producerOwner.AssertAccess();
	m_pendingUi = std::move(packet);
}

void RenderCoordinator::SubmitRenderingSettings(EngineRenderingSettingsState settings)
{
	m_producerOwner.AssertAccess();
	if (m_config.IsThreaded())
	{
		SubmitControl(RenderSettingsChangedCommand{std::move(settings)});
		return;
	}
	ApplyEngineRenderingSettingsStateToCVars(settings);
}

void RenderCoordinator::SubmitViewportRequest(const ViewportRenderRequest& request)
{
	m_producerOwner.AssertAccess();
	if (m_config.IsThreaded())
	{
		SubmitControl(RenderViewportCommand{request});
	}
	else
	{
		GetSerialContext().GetPipeline().SubmitViewportRenderRequest(request);
	}
}

void RenderCoordinator::RenderFrame()
{
	m_producerOwner.AssertAccess();
	if (!m_pendingInput)
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

RenderFramePacket RenderCoordinator::TakePendingFrame()
{
	RenderFramePacket packet{
	    .Input = std::move(*m_pendingInput),
	    .Timing = m_timer->GetTimeInfo(),
	    .Ui = m_pendingUi ? std::move(*m_pendingUi) : UiRenderPacket{}};

	m_pendingInput.reset();
	m_pendingUi.reset();
	return packet;
}

void RenderCoordinator::ExecuteSerialFrame()
{
	GetSerialContext().ExecuteFrame(TakePendingFrame());
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

	if (!m_frameQueue->Publish(*ticket, TakePendingFrame()))
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render frame queue rejected its producer-owned writing ticket.");
	}

	SubmitControl(RenderFrameReadyCommand{*ticket});

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
		return GetSerialContext().GetPipeline().GetViewportRenderProducts();
	}

	std::lock_guard lock(m_readStateMutex);
	return m_publishedViewportProducts;
}

void RenderCoordinator::ReloadCookedShaders()
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		GetSerialContext().GetRendererHost().ReloadCookedShaders();
		return;
	}

	(void) ExtractControlResult<std::monostate>(
	    SubmitSynchronousControl(RenderReloadShadersCommand{std::make_shared<RenderControlCompletion>()}));
}

std::uint64_t RenderCoordinator::GetShaderPackageGeneration() const noexcept
{
	m_producerOwner.AssertAccess();
	return m_config.IsThreaded() ? m_shaderPackageGeneration.load(std::memory_order_acquire)
	                             : GetSerialContext().GetRendererHost().GetShaderPackageGeneration();
}

MeshDiagnosticsSnapshot RenderCoordinator::CaptureMeshDiagnostics()
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		return GetSerialContext().GetRendererHost().CaptureMeshDiagnostics();
	}

	auto completion = std::make_shared<RenderControlCompletion>();
	return ExtractControlResult<MeshDiagnosticsSnapshot>(
	    SubmitSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Meshes, 0, completion}));
}

MeshPreviewGeometry RenderCoordinator::CaptureMeshPreview(std::uintptr_t meshRuntimeId)
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		return GetSerialContext().GetRendererHost().CaptureMeshPreview(meshRuntimeId);
	}

	auto completion = std::make_shared<RenderControlCompletion>();
	return ExtractControlResult<MeshPreviewGeometry>(
	    SubmitSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::MeshPreview, meshRuntimeId, completion}));
}

TextureDiagnosticsSnapshot RenderCoordinator::CaptureTextureDiagnostics()
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		return GetSerialContext().GetPipeline().CaptureTextureDiagnostics();
	}

	auto completion = std::make_shared<RenderControlCompletion>();
	return ExtractControlResult<TextureDiagnosticsSnapshot>(
	    SubmitSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Textures, 0, completion}));
}

RendererMemoryDiagnosticsSnapshot RenderCoordinator::CaptureMemoryDiagnostics()
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		return GetSerialContext().GetRendererHost().CaptureMemoryDiagnostics();
	}

	auto completion = std::make_shared<RenderControlCompletion>();
	return ExtractControlResult<RendererMemoryDiagnosticsSnapshot>(
	    SubmitSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Memory, 0, completion}));
}

ViewportCaptureId RenderCoordinator::RequestViewportCapture(ViewportCaptureRequest request)
{
	m_producerOwner.AssertAccess();
	const ViewportCaptureId id{m_nextViewportCaptureId++};
	if (m_config.IsThreaded())
	{
		SubmitControl(RenderCaptureCommand{id, std::move(request)});
		return id;
	}
	(void) GetSerialContext().GetPipeline().BeginViewportCapture(id, request);
	PublishReadState();
	return id;
}

bool RenderCoordinator::TryTakeViewportCapture(ViewportCaptureReadback& readback)
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		PublishReadState();
	}
	std::lock_guard lock(m_readStateMutex);
	if (m_publishedViewportCaptures.empty())
	{
		return false;
	}
	readback = std::move(m_publishedViewportCaptures.front());
	m_publishedViewportCaptures.erase(m_publishedViewportCaptures.begin());
	return true;
}

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

		while (const std::optional<RenderControlCommand> command = m_controlQueue->WaitPop())
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
		m_context->ExecuteControl(command.Payload);
		PublishReadState();
	}
}

void RenderCoordinator::ExecuteThreadedFrame(RenderFrameQueueTicket ticket)
{
	RenderFramePacket packet;
	if (!m_frameQueue->Consume(ticket, packet))
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render frame queue rejected a queued frame ticket.");
	}
	m_context->ExecuteFrame(std::move(packet));
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

void RenderCoordinator::PublishReadState()
{
	if (m_context == nullptr)
	{
		return;
	}

	{
		std::lock_guard lock(m_readStateMutex);
		m_publishedViewportProducts = m_context->GetPipeline().GetViewportRenderProducts();
		std::vector<ViewportCaptureReadback> captures = m_context->GetPipeline().TakeCompletedViewportCaptures();
		for (ViewportCaptureReadback& capture : captures)
		{
			if (m_publishedViewportCaptures.size() >= 3)
			{
				m_publishedViewportCaptures.erase(m_publishedViewportCaptures.begin());
			}
			m_publishedViewportCaptures.push_back(std::move(capture));
		}
	}
	m_shaderPackageGeneration.store(m_context->GetRendererHost().GetShaderPackageGeneration(), std::memory_order_release);
}

void RenderCoordinator::SubmitControl(RenderControlPayload payload)
{
	m_producerOwner.AssertAccess();
	const std::uint64_t sequenceNumber = IssueControlSequence();
	m_controlQueue->WaitPush(RenderControlCommand{sequenceNumber, std::move(payload)});
}

template <typename TCommand> RenderControlResult RenderCoordinator::SubmitSynchronousControl(TCommand command)
{
	const std::shared_ptr<RenderControlCompletion> completion = command.Completion;
	if (!completion)
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Synchronous render-control command has no completion owner.");
	}
	SubmitControl(RenderControlPayload{std::move(command)});
	return completion->Wait();
}

std::uint64_t RenderCoordinator::IssueControlSequence() noexcept
{
	if (m_nextControlSequence == 0)
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render-control command sequence identity exhausted.");
	}

	const std::uint64_t sequenceNumber = m_nextControlSequence;
	m_nextControlSequence = sequenceNumber == (std::numeric_limits<std::uint64_t>::max)() ? 0 : sequenceNumber + 1;
	return sequenceNumber;
}

void RenderCoordinator::SubmitResize()
{
	const RenderResizeCommand command{{m_window->GetWidth(), m_window->GetHeight()}, m_window->IsMinimized()};
	if (m_config.IsThreaded())
	{
		SubmitControl(command);
	}
	else if (m_context)
	{
		m_context->GetPipeline().RequestResize(command.Extent, command.Minimized);
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
