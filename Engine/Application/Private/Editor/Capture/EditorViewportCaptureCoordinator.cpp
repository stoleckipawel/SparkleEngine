#include "PCH.h"

#include "Editor/Capture/EditorViewportCaptureCoordinator.h"

#include "Core/Public/FileSystemUtils.h"
#include "Editor/Capture/ViewportCaptureWriter.h"
#include "Renderer.h"

EditorViewportCaptureCoordinator::EditorViewportCaptureCoordinator(EditorOperationRuntime& operations) noexcept :
    m_writeOperation(operations)
{
}

void EditorViewportCaptureCoordinator::Request(Renderer& renderer, std::uint64_t frameId)
{
	if (m_activeCapture)
	{
		return;
	}
	m_outputPath = BuildOutputPath(frameId);
	m_activeCapture =
	    renderer.RequestViewportCapture(ViewportCaptureRequest{.Output = RenderOutputFlags::SceneColor, .ExpectedFrameId = 0});
	if (!m_activeCapture)
	{
		m_lastResult.Status = ViewportCaptureStatus::Failed;
		m_lastResult.FailureReason = "Viewport capture capacity is exhausted.";
	}
}

void EditorViewportCaptureCoordinator::Update(Renderer& renderer)
{
	ViewportCaptureReadback readback;
	if (m_activeCapture && renderer.TryTakeViewportCapture(m_activeCapture, readback))
	{
		m_activeCapture = {};
		if (!readback.Result)
		{
			m_lastResult = std::move(readback.Result);
		}
		else
		{
			std::string errorMessage;
			if (!m_writeOperation.Start(
			        TaskName("Write viewport capture"),
			        "A viewport capture is already being written.",
			        [readback = std::move(readback),
			            outputPath = m_outputPath](ViewportCaptureResult& result, TaskExecutionContext& context) mutable
			        {
				        result = ViewportCaptureWriter::Write(std::move(readback), outputPath, context.GetCancellationToken());
				        return result ? TaskResult::Success()
				                      : (context.IsCancellationRequested() ? TaskResult::Cancelled(result.FailureReason)
				                                                           : TaskResult::Failure(result.FailureReason));
			        },
			        errorMessage))
			{
				m_lastResult.Status = ViewportCaptureStatus::Failed;
				m_lastResult.FailureReason = std::move(errorMessage);
			}
		}
	}

	ViewportCaptureResult result;
	if (m_writeOperation.TryConsume(result))
	{
		m_lastResult = std::move(result);
	}
}

std::filesystem::path EditorViewportCaptureCoordinator::BuildOutputPath(std::uint64_t frameId) const
{
	return Filesystem::GetWorkspaceRootPath() / "Saved" / "Captures" / ("Viewport_" + std::to_string(frameId) + ".bmp");
}
