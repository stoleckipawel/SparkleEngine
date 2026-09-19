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
    ReferencePathTracerArtifactKind kind,
    Renderer& renderer,
    const ViewportRenderProducts& products,
    const std::filesystem::path& outputRoot,
    std::uint64_t maximumOutputBytes)
{
	if (!IsSettled())
	{
		return;
	}
	if (products.GetProgress().State == ViewportRenderProgressState::Unavailable)
	{
		Fail("The current viewport does not own an available Reference Path Tracer prefix.");
		return;
	}
	m_maximumOutputBytes = maximumOutputBytes;
	m_outputRoot = outputRoot.empty() ? DefaultOutputRoot() : outputRoot;
	if (kind == ReferencePathTracerArtifactKind::Complete)
	{
		Begin(kind, renderer, products, m_outputRoot);
		return;
	}
	const RenderProduct* mean = products.FindProduct(RenderOutputFlags::Radiance);
	if (mean == nullptr || mean->SamplePrefix.SampleCount == 0u)
	{
		Fail("No committed radiance sample prefix is available for publication.");
		return;
	}
	m_pendingKind = kind;
	m_pendingPrefix = mean->SamplePrefix;
}

void ReferencePathTracerArtifactCoordinator::Begin(
    ReferencePathTracerArtifactKind kind,
    Renderer& renderer,
    const ViewportRenderProducts& products,
    const std::filesystem::path& outputRoot)
{
	const RenderProduct* mean = products.FindProduct(RenderOutputFlags::Radiance);
	const ViewportRenderProgress& progress = products.GetProgress();
	if (mean == nullptr || mean->SamplePrefix.SampleCount == 0u)
	{
		Fail("No committed radiance sample prefix is available for publication.");
		return;
	}
	if (kind == ReferencePathTracerArtifactKind::Complete && progress.State != ViewportRenderProgressState::Complete)
	{
		Fail("The requested complete prefix has not reached its target.");
		return;
	}

	m_kind = kind;
	m_outputRoot = outputRoot;
	const std::string digest = Hash::Sha256ToHex(mean->SamplePrefix.RenderIdentitySha256);
	const char* label = kind == ReferencePathTracerArtifactKind::Complete
	    ? "Complete"
	    : (kind == ReferencePathTracerArtifactKind::Checkpoint ? "Checkpoint" : "PartialPrefix");
	m_publicationDirectory =
	    m_outputRoot / digest / (Identifiers::CreateUuidV4String() + "-" + label + "-" + std::to_string(mean->SamplePrefix.SampleCount));
	m_lastResult = {};
	m_mean.reset();
	m_moment2.reset();
	m_capturePrefix = mean->SamplePrefix;
	if (!m_meanCapture.Request(renderer, ViewportCaptureRequest{.Output = RenderOutputFlags::Radiance}))
	{
		Fail("The renderer did not accept the Reference radiance readback request.");
		return;
	}
	if (kind == ReferencePathTracerArtifactKind::Checkpoint)
	{
		if (!m_moment2Capture.Request(renderer, ViewportCaptureRequest{.Output = RenderOutputFlags::RadianceSecondMoment}))
		{
			Fail("The renderer did not accept the checkpoint M2 readback request.");
		}
	}
}

void ReferencePathTracerArtifactCoordinator::CollectReadback(
    Renderer& renderer,
    ViewportCaptureSlot& capture,
    std::optional<ViewportCaptureReadback>& destination)
{
	capture.Update(renderer);
	if (!capture.HasReadback())
	{
		return;
	}
	ViewportCaptureReadback readback = capture.TakeReadback();
	if (!readback.Result)
	{
		Fail(std::move(readback.Result.FailureReason));
		return;
	}
	if (readback.Result.SamplePrefix != m_capturePrefix)
	{
		Fail("The radiance readback no longer belongs to the requested immutable sample prefix.");
		return;
	}
	destination = std::move(readback);
}

void ReferencePathTracerArtifactCoordinator::PublishIfReady()
{
	if (!m_mean || (m_kind == ReferencePathTracerArtifactKind::Checkpoint && !m_moment2))
	{
		return;
	}
	if (m_moment2
	    && (m_mean->Result.SamplePrefix.RenderIdentitySha256 != m_moment2->Result.SamplePrefix.RenderIdentitySha256
	        || m_mean->Result.SamplePrefix.SampleCount != m_moment2->Result.SamplePrefix.SampleCount || m_mean->Width != m_moment2->Width
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
}

void ReferencePathTracerArtifactCoordinator::Update(Renderer& renderer, const ViewportRenderProducts& products)
{
	CollectReadback(renderer, m_meanCapture, m_mean);
	CollectReadback(renderer, m_moment2Capture, m_moment2);
	PublishIfReady();

	if (m_pendingKind)
	{
		const RenderProduct* mean = products.FindProduct(RenderOutputFlags::Radiance);
		if (mean == nullptr || mean->SamplePrefix.RenderIdentitySha256 != m_pendingPrefix.RenderIdentitySha256
		    || mean->SamplePrefix.TargetSampleCount != m_pendingPrefix.TargetSampleCount)
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
	ReferencePathTracerArtifactWriteResult result;
	if (m_writeOperation.TryConsume(result))
	{
		m_lastResult = std::move(result);
	}
}

bool ReferencePathTracerArtifactCoordinator::IsSettled() const noexcept
{
	return !m_pendingKind && m_meanCapture.IsSettled() && m_moment2Capture.IsSettled() && !m_writeOperation.IsOccupied();
}

void ReferencePathTracerArtifactCoordinator::Fail(std::string message)
{
	m_meanCapture.Discard();
	m_moment2Capture.Discard();
	m_mean.reset();
	m_moment2.reset();
	m_pendingKind.reset();
	m_capturePrefix = {};
	m_lastResult = ReferencePathTracerArtifactWriteResult{
	    .Succeeded = false,
	    .PublicationDirectory = m_publicationDirectory,
	    .ErrorMessage = std::move(message)};
}

std::filesystem::path ReferencePathTracerArtifactCoordinator::DefaultOutputRoot()
{
	return Filesystem::GetWorkspaceRootPath() / "Saved" / "ReferencePathTracer";
}
