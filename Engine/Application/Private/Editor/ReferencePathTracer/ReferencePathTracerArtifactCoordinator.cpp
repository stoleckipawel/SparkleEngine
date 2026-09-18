#include "PCH.h"

#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactCoordinator.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Identifiers/Uuid.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactWriter.h"
#include "Renderer.h"

#include <utility>

ReferencePathTracerArtifactCoordinator::ReferencePathTracerArtifactCoordinator(EditorOperationRuntime& operations) noexcept :
    m_writeOperation(operations)
{
}

void ReferencePathTracerArtifactCoordinator::Request(
    ReferencePathTracerOutputAction action,
    Renderer& renderer,
    const ViewportRenderProducts& products,
    const std::filesystem::path& outputRoot,
    std::uint64_t maximumOutputBytes)
{
	if (action == ReferencePathTracerOutputAction::None || !IsSettled())
	{
		return;
	}
	if (products.GetProgress().State == ViewportRenderProgressState::Unavailable)
	{
		Fail("The current viewport does not own an available Reference Path Tracer prefix.");
		return;
	}
	if (action == ReferencePathTracerOutputAction::SaveWhenComplete)
	{
		const RenderProduct* mean = products.FindProduct(RenderOutputFlags::RawSceneColor);
		if (mean == nullptr || mean->Source.TargetWork == 0u)
		{
			Fail("No active raw session is available for a save-when-complete intent.");
			return;
		}
		m_saveWhenComplete = true;
		m_saveWhenCompleteSource = mean->Source;
		m_outputRoot = outputRoot.empty() ? DefaultOutputRoot() : outputRoot;
		m_maximumOutputBytes = maximumOutputBytes;
		return;
	}

	const ReferencePathTracerArtifactKind kind = action == ReferencePathTracerOutputAction::SaveCheckpoint
	    ? ReferencePathTracerArtifactKind::Checkpoint
	    : (action == ReferencePathTracerOutputAction::SaveComplete ? ReferencePathTracerArtifactKind::Complete
	                                                               : ReferencePathTracerArtifactKind::PartialPrefix);
	m_maximumOutputBytes = maximumOutputBytes;
	m_outputRoot = outputRoot.empty() ? DefaultOutputRoot() : outputRoot;
	if (kind == ReferencePathTracerArtifactKind::Complete)
	{
		Begin(kind, renderer, products, m_outputRoot);
		return;
	}
	const RenderProduct* mean = products.FindProduct(RenderOutputFlags::RawSceneColor);
	if (mean == nullptr || mean->Source.CommittedWork == 0u)
	{
		Fail("No committed raw prefix is available for publication.");
		return;
	}
	m_pendingKind = kind;
	m_pendingSource = mean->Source;
}

void ReferencePathTracerArtifactCoordinator::Begin(
    ReferencePathTracerArtifactKind kind,
    Renderer& renderer,
    const ViewportRenderProducts& products,
    const std::filesystem::path& outputRoot)
{
	const RenderProduct* mean = products.FindProduct(RenderOutputFlags::RawSceneColor);
	const ViewportRenderProgress& progress = products.GetProgress();
	if (mean == nullptr || mean->Source.CommittedWork == 0u)
	{
		Fail("No committed raw prefix is available for publication.");
		return;
	}
	if (kind == ReferencePathTracerArtifactKind::Complete && progress.State != ViewportRenderProgressState::Complete)
	{
		Fail("The requested complete prefix has not reached its target.");
		return;
	}

	m_kind = kind;
	m_outputRoot = outputRoot;
	const std::string digest = Hash::Sha256ToHex(mean->Source.IdentitySha256);
	const char* label = kind == ReferencePathTracerArtifactKind::Complete
	    ? "Complete"
	    : (kind == ReferencePathTracerArtifactKind::Checkpoint ? "Checkpoint" : "PartialPrefix");
	m_publicationDirectory =
	    m_outputRoot / digest / (Identifiers::CreateUuidV4String() + "-" + label + "-" + std::to_string(mean->Source.CommittedWork));
	m_lastResult = {};
	m_mean.reset();
	m_moment2.reset();
	m_captureSource = mean->Source;
	m_meanCapture = renderer.RequestViewportCapture(ViewportCaptureRequest{.Output = RenderOutputFlags::RawSceneColor});
	if (!m_meanCapture)
	{
		Fail("The renderer did not accept the raw prefix readback request.");
		return;
	}
	if (kind == ReferencePathTracerArtifactKind::Checkpoint)
	{
		m_moment2Capture = renderer.RequestViewportCapture(ViewportCaptureRequest{.Output = RenderOutputFlags::RawSceneColorMoment2});
		if (!m_moment2Capture)
		{
			Fail("The renderer did not accept the checkpoint M2 readback request.");
		}
	}
}

void ReferencePathTracerArtifactCoordinator::CollectReadback(
    Renderer& renderer,
    ViewportCaptureId& capture,
    std::optional<ViewportCaptureReadback>& destination)
{
	if (!capture)
	{
		return;
	}
	ViewportCaptureReadback readback;
	if (!renderer.TryTakeViewportCapture(capture, readback))
	{
		return;
	}
	capture = {};
	if (!readback.Result)
	{
		Fail(std::move(readback.Result.FailureReason));
		return;
	}
	if (readback.Result.Source != m_captureSource)
	{
		Fail("The raw readback no longer belongs to the requested immutable prefix.");
		return;
	}
	destination = std::move(readback);
}

void ReferencePathTracerArtifactCoordinator::DrainDiscardedReadbacks(Renderer& renderer)
{
	for (std::size_t index = 0; index < m_discardedCaptures.size();)
	{
		ViewportCaptureReadback readback;
		if (!renderer.TryTakeViewportCapture(m_discardedCaptures[index], readback))
		{
			++index;
			continue;
		}
		m_discardedCaptures.erase(m_discardedCaptures.begin() + index);
	}
}

void ReferencePathTracerArtifactCoordinator::PublishIfReady()
{
	if (!m_mean || (m_kind == ReferencePathTracerArtifactKind::Checkpoint && !m_moment2))
	{
		return;
	}
	if (m_moment2
	    && (m_mean->Result.Source.IdentitySha256 != m_moment2->Result.Source.IdentitySha256
	        || m_mean->Result.Source.CommittedWork != m_moment2->Result.Source.CommittedWork || m_mean->Width != m_moment2->Width
	        || m_mean->Height != m_moment2->Height))
	{
		Fail("Checkpoint planes do not describe the same immutable session prefix.");
		return;
	}

	std::string errorMessage;
	ReferencePathTracerArtifactWriteRequest request{
	    .Kind = m_kind,
	    .AllowedRoot = m_outputRoot,
	    .PublicationDirectory = m_publicationDirectory,
	    .MaximumOutputBytes = m_maximumOutputBytes,
	    .Mean = std::move(*m_mean),
	    .Moment2 = std::move(m_moment2)};
	m_mean.reset();
	m_moment2.reset();
	if (!m_writeOperation.Start(
	        TaskName("Publish Reference Path Tracer artifact"),
	        "A Reference Path Tracer artifact is already being written.",
	        [request = std::move(request)](ReferencePathTracerArtifactWriteResult& result, TaskExecutionContext& context) mutable
	        {
		        if (context.IsCancellationRequested())
		        {
			        return TaskResult::Cancelled("Reference Path Tracer artifact publication was cancelled.");
		        }
		        result = ReferencePathTracerArtifactWriter::Write(std::move(request), context.GetCancellationToken());
		        if (context.IsCancellationRequested())
		        {
			        return TaskResult::Cancelled("Reference Path Tracer artifact publication was cancelled.");
		        }
		        return result.Succeeded ? TaskResult::Success() : TaskResult::Failure(result.ErrorMessage);
	        },
	        errorMessage))
	{
		Fail(std::move(errorMessage));
		return;
	}
	m_writeActive = true;
}

void ReferencePathTracerArtifactCoordinator::Update(Renderer& renderer, const ViewportRenderProducts& products)
{
	DrainDiscardedReadbacks(renderer);
	CollectReadback(renderer, m_meanCapture, m_mean);
	CollectReadback(renderer, m_moment2Capture, m_moment2);
	PublishIfReady();

	if (m_pendingKind)
	{
		const RenderProduct* mean = products.FindProduct(RenderOutputFlags::RawSceneColor);
		if (mean == nullptr || mean->Source.IdentitySha256 != m_pendingSource.IdentitySha256
		    || mean->Source.TargetWork != m_pendingSource.TargetWork)
		{
			Fail("The output intent was cancelled because the session identity changed before its prefix settled.");
			return;
		}
		const ViewportRenderProgressState state = products.GetProgress().State;
		if (state == ViewportRenderProgressState::Unavailable)
		{
			Fail("The output intent could not settle because the session became unavailable.");
			return;
		}
		if (state == ViewportRenderProgressState::Paused || state == ViewportRenderProgressState::Complete)
		{
			const ReferencePathTracerArtifactKind kind = *m_pendingKind;
			m_pendingKind.reset();
			Begin(kind, renderer, products, m_outputRoot);
		}
	}
	if (m_saveWhenComplete)
	{
		const RenderProduct* mean = products.FindProduct(RenderOutputFlags::RawSceneColor);
		if (mean == nullptr || mean->Source.IdentitySha256 != m_saveWhenCompleteSource.IdentitySha256
		    || mean->Source.TargetWork != m_saveWhenCompleteSource.TargetWork)
		{
			m_saveWhenComplete = false;
			Fail("The save-when-complete intent was cancelled because the session identity changed.");
			return;
		}
		if (products.GetProgress().State == ViewportRenderProgressState::Unavailable)
		{
			Fail("The save-when-complete intent failed because the session became unavailable.");
			return;
		}
	}
	if (m_saveWhenComplete && !m_meanCapture && !m_moment2Capture && !m_writeActive
	    && products.GetProgress().State == ViewportRenderProgressState::Complete)
	{
		m_saveWhenComplete = false;
		Begin(ReferencePathTracerArtifactKind::Complete, renderer, products, m_outputRoot);
	}
	if (!m_writeActive)
	{
		return;
	}
	ReferencePathTracerArtifactWriteResult result;
	if (m_writeOperation.TryConsume(result))
	{
		m_writeActive = false;
		m_lastResult = std::move(result);
	}
}

bool ReferencePathTracerArtifactCoordinator::IsSettled() const noexcept
{
	return !m_saveWhenComplete && !m_pendingKind && !m_meanCapture && !m_moment2Capture && m_discardedCaptures.empty() && !m_writeActive;
}

void ReferencePathTracerArtifactCoordinator::Fail(std::string message)
{
	m_saveWhenComplete = false;
	if (m_meanCapture)
	{
		m_discardedCaptures.push_back(m_meanCapture);
	}
	if (m_moment2Capture)
	{
		m_discardedCaptures.push_back(m_moment2Capture);
	}
	m_meanCapture = {};
	m_moment2Capture = {};
	m_mean.reset();
	m_moment2.reset();
	m_pendingKind.reset();
	m_writeActive = false;
	m_captureSource = {};
	m_lastResult = ReferencePathTracerArtifactWriteResult{
	    .Succeeded = false,
	    .PublicationDirectory = m_publicationDirectory,
	    .ErrorMessage = std::move(message)};
}

std::filesystem::path ReferencePathTracerArtifactCoordinator::DefaultOutputRoot()
{
	return Filesystem::GetWorkspaceRootPath() / "Saved" / "ReferencePathTracer";
}
