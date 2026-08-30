#include "PCH.h"
#include "Concurrency/Coordinator/RenderCoordinator.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Concurrency/Coordinator/RendererExecutionContext.h"
#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Frame/FramePipeline.h"
#include "Host/RendererHost.h"
#include "Pipeline/RenderPassRuntimeCache.h"

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

void RenderCoordinator::ReloadShaders()
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		GetSerialContext().GetRendererHost().GetRenderPassRuntimeCache().ReloadShaders();
		return;
	}

	(void) ExtractControlResult<std::monostate>(
	    SubmitSynchronousControl(RenderReloadShadersCommand{std::make_shared<RenderControlCompletion>()}));
}

std::uint64_t RenderCoordinator::GetShaderGeneration() const noexcept
{
	m_producerOwner.AssertAccess();
	return m_config.IsThreaded() ? m_shaderGeneration.load(std::memory_order_acquire)
	                             : GetSerialContext().GetRendererHost().GetRenderPassRuntimeCache().GetShaderGeneration();
}

MeshDiagnosticsSnapshot RenderCoordinator::CaptureMeshDiagnostics()
{
	m_producerOwner.AssertAccess();
	if (!m_config.IsThreaded())
	{
		RendererHost& host = GetSerialContext().GetRendererHost();
		return MeshDiagnosticsCollector::Capture(host.GetRenderScene(), &host.GetGpuMeshCache());
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
		RendererHost& host = GetSerialContext().GetRendererHost();
		return MeshDiagnosticsCollector::CapturePreview(host.GetRenderScene(), meshRuntimeId);
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
		return GetSerialContext().GetRendererHost().GetMemoryMonitor().GetLatestSnapshot();
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
	m_shaderGeneration.store(m_context->GetRendererHost().GetRenderPassRuntimeCache().GetShaderGeneration(), std::memory_order_release);
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
