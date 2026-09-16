#include "PCH.h"
#include "Concurrency/Coordinator/RenderCoordinator.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Concurrency/Coordinator/RendererExecutionContext.h"

#include <limits>

static const auto g_renderCoordinatorLogger = Logging::GetOrCreateLogger("Renderer.Coordinator");

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
	(void) ExtractControlResult<std::monostate>(
	    ExecuteSynchronousControl(RenderReloadShadersCommand{std::make_shared<RenderControlCompletion>()}));
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
	return ExtractControlResult<MeshDiagnosticsSnapshot>(
	    ExecuteSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Meshes, 0, completion}));
}

MeshPreviewGeometry RenderCoordinator::CaptureMeshPreview(std::uintptr_t meshRuntimeId)
{
	m_producerOwner.AssertAccess();
	auto completion = std::make_shared<RenderControlCompletion>();
	return ExtractControlResult<MeshPreviewGeometry>(
	    ExecuteSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::MeshPreview, meshRuntimeId, completion}));
}

TextureDiagnosticsSnapshot RenderCoordinator::CaptureTextureDiagnostics()
{
	m_producerOwner.AssertAccess();
	auto completion = std::make_shared<RenderControlCompletion>();
	return ExtractControlResult<TextureDiagnosticsSnapshot>(
	    ExecuteSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Textures, 0, completion}));
}

RendererMemoryDiagnosticsSnapshot RenderCoordinator::CaptureMemoryDiagnostics()
{
	m_producerOwner.AssertAccess();
	auto completion = std::make_shared<RenderControlCompletion>();
	return ExtractControlResult<RendererMemoryDiagnosticsSnapshot>(
	    ExecuteSynchronousControl(RenderDiagnosticsCommand{RenderDiagnosticsRequestKind::Memory, 0, completion}));
}

ViewportCaptureId RenderCoordinator::RequestViewportCapture(ViewportCaptureRequest request)
{
	m_producerOwner.AssertAccess();
	const ViewportCaptureId id{m_nextViewportCaptureId++};
	DispatchControl(RenderCaptureCommand{id, std::move(request)});
	if (!m_config.IsThreaded())
	{
		PublishReadState();
	}
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

void RenderCoordinator::PublishReadState()
{
	if (m_context == nullptr)
	{
		return;
	}

	{
		std::lock_guard lock(m_readStateMutex);
		m_publishedViewportProducts = m_context->GetViewportRenderProducts();
		std::vector<ViewportCaptureReadback> captures = m_context->TakeCompletedViewportCaptures();
		for (ViewportCaptureReadback& capture : captures)
		{
			if (m_publishedViewportCaptures.size() >= 3)
			{
				m_publishedViewportCaptures.erase(m_publishedViewportCaptures.begin());
			}
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

void RenderCoordinator::SubmitThreadCommand(RenderThreadCommandPayload payload)
{
	m_producerOwner.AssertAccess();
	const std::uint64_t sequenceNumber = IssueThreadCommandSequence();
	m_threadCommandQueue->WaitPush(RenderThreadCommand{sequenceNumber, std::move(payload)});
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
