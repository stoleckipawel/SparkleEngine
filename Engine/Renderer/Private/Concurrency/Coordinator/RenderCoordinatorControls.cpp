#include "PCH.h"
#include "Concurrency/Coordinator/RenderCoordinator.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Concurrency/Coordinator/RendererExecutionContext.h"

#include <algorithm>
#include <limits>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_renderCoordinatorLogger, "Renderer.Coordinator");

template <typename TResult> TResult RenderCoordinator::ExtractControlResult(RenderControlResult result)
{
	if (RenderControlError* error = std::get_if<RenderControlError>(&result))
	{
		throw Diagnostics::Error(error->Message);
	}
	if (TResult* value = std::get_if<TResult>(&result))
	{
		return std::move(*value);
	}
	Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render control returned an incompatible payload.");
}

void RenderCoordinator::ReloadShaders()
{
	m_producerOwner.AssertAccess();

	(void) ExtractControlResult<std::monostate>(ExecuteSynchronousControl(RenderReloadShadersCommand{std::make_shared<RenderControlCompletion>()}));
}

std::uint64_t RenderCoordinator::GetShaderGeneration() const noexcept
{
	m_producerOwner.AssertAccess();
	return m_config.IsThreaded() ? m_shaderGeneration.load(std::memory_order_acquire) : GetSerialContext().GetShaderGeneration();
}

MeshDiagnosticsSnapshot RenderCoordinator::CaptureMeshDiagnostics()
{
	m_producerOwner.AssertAccess();
	auto completion = std::make_shared<RenderControlCompletion>();

	return ExtractControlResult<MeshDiagnosticsSnapshot>(ExecuteSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Meshes, 0, completion}));
}

MeshPreviewGeometry RenderCoordinator::CaptureMeshPreview(std::uintptr_t meshRuntimeId)
{
	m_producerOwner.AssertAccess();
	auto completion = std::make_shared<RenderControlCompletion>();

	return ExtractControlResult<MeshPreviewGeometry>(ExecuteSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::MeshPreview, meshRuntimeId, completion}));
}

TextureDiagnosticsSnapshot RenderCoordinator::CaptureTextureDiagnostics()
{
	m_producerOwner.AssertAccess();
	auto completion = std::make_shared<RenderControlCompletion>();

	return ExtractControlResult<TextureDiagnosticsSnapshot>(ExecuteSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Textures, 0, completion}));
}

RendererMemoryDiagnosticsSnapshot RenderCoordinator::CaptureMemoryDiagnostics()
{
	m_producerOwner.AssertAccess();
	auto completion = std::make_shared<RenderControlCompletion>();

	return ExtractControlResult<RendererMemoryDiagnosticsSnapshot>(ExecuteSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Memory, 0, completion}));
}

ViewportCaptureAdmission RenderCoordinator::RequestViewportCapture(ViewportCaptureRequest request)
{
	m_producerOwner.AssertAccess();
	std::unique_lock lock(m_readStateMutex);

	const auto slot = std::find_if(m_outstandingViewportCaptures.begin(), m_outstandingViewportCaptures.end(), [](ViewportCaptureId candidate) { return !candidate; });

	if (slot == m_outstandingViewportCaptures.end())
	{
		return {.Status = ViewportCaptureAdmissionStatus::Full};
	}
	if (m_nextViewportCaptureId == 0)
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Viewport capture identity exhausted.");
	}
	const ViewportCaptureId id{m_nextViewportCaptureId};
	if (m_config.IsThreaded())
	{
		const RenderThreadCommandAdmission admission = m_threadCommandQueue->TryPush(
		    RenderThreadCommand{IssueThreadCommandSequence(), RendererExecutionControl{RenderCaptureCommand{id, std::move(request)}}});

		if (admission == RenderThreadCommandAdmission::Full)
		{
			return {.Status = ViewportCaptureAdmissionStatus::Full};
		}
		if (admission == RenderThreadCommandAdmission::Closed)
		{
			return {.Status = ViewportCaptureAdmissionStatus::Closed};
		}
	}
	*slot = id;
	++m_nextViewportCaptureId;
	lock.unlock();
	if (!m_config.IsThreaded())
	{
		GetSerialContext().ExecuteControl(RenderCaptureCommand{id, std::move(request)});
		PublishReadState();
	}
	return {.Status = ViewportCaptureAdmissionStatus::Accepted, .Id = id};
}

bool RenderCoordinator::TryTakeViewportCapture(ViewportCaptureId id, ViewportCaptureReadback& readback)
{
	m_producerOwner.AssertAccess();
	if (!id)
	{
		return false;
	}
	if (!m_config.IsThreaded())
	{
		PublishReadState();
	}

	std::lock_guard lock(m_readStateMutex);

	const auto completion = std::find_if(
	    m_publishedViewportCaptures.begin(),
	    m_publishedViewportCaptures.end(),
	    [id](const ViewportCaptureCompletion& candidate) { return candidate.Id.Value == id.Value; });

	if (completion == m_publishedViewportCaptures.end())
	{
		return false;
	}
	readback = std::move(completion->Readback);
	m_publishedViewportCaptures.erase(completion);

	const auto slot = std::find_if(m_outstandingViewportCaptures.begin(), m_outstandingViewportCaptures.end(), [id](ViewportCaptureId candidate) { return candidate.Value == id.Value; });

	if (slot == m_outstandingViewportCaptures.end())
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Viewport completion has no outstanding capture owner.");
	}
	*slot = {};
	return true;
}

void RenderCoordinator::PublishReadState()
{
	if (m_context == nullptr)
	{
		return;
	}

	{
		std::lock_guard lock(m_readStateMutex);
		if (m_publishedViewportPresentation.PublicationSequence == (std::numeric_limits<std::uint64_t>::max)())
		{
			Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Viewport publication identity exhausted.");
		}

		m_publishedViewportPresentation = ViewportPresentationSnapshot{
		    .Products = m_context->GetViewportRenderProducts(),
		    .Texture = m_context->GetViewportPresentationTexture(),
		    .PublicationSequence = m_publishedViewportPresentation.PublicationSequence + 1};

		std::vector<ViewportCaptureCompletion> captures = m_context->TakeCompletedViewportCaptures();
		for (ViewportCaptureCompletion& capture : captures)
		{
			m_publishedViewportCaptures.push_back(std::move(capture));
		}
	}
	m_shaderGeneration.store(m_context->GetShaderGeneration(), std::memory_order_release);
}

void RenderCoordinator::DispatchControl(RendererExecutionControl control)
{
	if (m_config.IsThreaded())
	{
		SubmitThreadCommand(std::move(control));
	}
	else
	{
		GetSerialContext().ExecuteControl(std::move(control));
	}
}

CVarControlResult RenderCoordinator::ExecuteConsoleVariables(CVarControlRequest request)
{
	m_producerOwner.AssertAccess();

	RenderControlResult result = ExecuteSynchronousControl(RenderCVarCommand{std::move(request), std::make_shared<RenderControlCompletion>()});

	if (auto* error = std::get_if<RenderControlError>(&result))
	{
		return {.Error = std::move(error->Message)};
	}
	return std::get<CVarControlResult>(std::move(result));
}

EngineRenderingSettingsState RenderCoordinator::CaptureRenderingSettings()
{
	m_producerOwner.AssertAccess();

	return ExtractControlResult<EngineRenderingSettingsState>(ExecuteSynchronousControl(RenderSettingsCaptureCommand{std::make_shared<RenderControlCompletion>()}));
}

void RenderCoordinator::SubmitThreadCommand(RenderThreadCommandPayload payload)
{
	m_producerOwner.AssertAccess();
	const std::uint64_t sequenceNumber = IssueThreadCommandSequence();
	std::shared_ptr<RenderControlCompletion> completion;
	bool shutdown = false;
	if (const auto* control = std::get_if<RendererExecutionControl>(&payload))
	{
		shutdown = std::holds_alternative<RenderShutdownCommand>(*control);

		std::visit(
		    [&completion](const auto& command)
		    {
			    if constexpr (requires { command.Completion; })
			    {
				    completion = command.Completion;
			    }
		    },
		    *control);
	}
	if (!m_threadCommandQueue->WaitPush(RenderThreadCommand{sequenceNumber, std::move(payload)}))
	{
		if (completion)
		{
			completion->Cancel();
		}
		else if (!shutdown)
		{
			Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render-thread command queue rejected submission after closing.");
		}
	}
}

template <typename TCommand> RenderControlResult RenderCoordinator::ExecuteSynchronousControl(TCommand command)
{
	const std::shared_ptr<RenderControlCompletion> completion = command.Completion;
	if (!completion)
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Synchronous render-control command has no completion owner.");
	}
	DispatchControl(RendererExecutionControl{std::move(command)});
	return completion->Wait();
}

std::uint64_t RenderCoordinator::IssueThreadCommandSequence() noexcept
{
	if (m_nextThreadCommandSequence == 0)
	{
		Diagnostics::Fatal(g_renderCoordinatorLogger, __FILE__, __LINE__, "Render-thread command sequence identity exhausted.");
	}

	const std::uint64_t sequenceNumber = m_nextThreadCommandSequence;
	m_nextThreadCommandSequence = sequenceNumber == (std::numeric_limits<std::uint64_t>::max)() ? 0 : sequenceNumber + 1;
	return sequenceNumber;
}

ExternalCaptureAdmission RenderCoordinator::RequestExternalCapture(std::uint64_t viewportGeneration) noexcept
{
	m_producerOwner.AssertAccess();
#if SPARKLE_WITH_EXTERNAL_CAPTURE
	RhiExternalCapture* capture = m_deviceLaunch.ExternalCapture;
	if (!capture)
	{
		return {};
	}
	const auto admission = capture->Reserve(viewportGeneration);
	if (admission.Status != ExternalCaptureAdmissionStatus::Accepted)
	{
		return admission;
	}
	RenderExternalCaptureCommand command{admission.RequestId};
	if (m_config.IsThreaded())
	{
		const auto result = m_threadCommandQueue->TryPush(RenderThreadCommand{IssueThreadCommandSequence(), RendererExecutionControl{command}});

		if (result != RenderThreadCommandAdmission::Accepted)
		{
			capture->Reject(admission.RequestId, "Render command queue rejected capture admission.");

			return {.Status = result == RenderThreadCommandAdmission::Full ? ExternalCaptureAdmissionStatus::Full : ExternalCaptureAdmissionStatus::Closed};
		}
	}
	else
	{
		GetSerialContext().ExecuteControl(command);
	}
	return admission;
#else
	(void) viewportGeneration;
	return {};
#endif
}
