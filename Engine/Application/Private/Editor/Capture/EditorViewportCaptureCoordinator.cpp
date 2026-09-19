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
	if (!m_capture.IsSettled() || m_writeOperation.IsOccupied())
	{
		return;
	}
	m_outputPath = BuildOutputPath(frameId);
	m_capture.Request(renderer, ViewportCaptureRequest{.Output = RenderOutputFlags::FinalColorLdr});
}

void EditorViewportCaptureCoordinator::Update(Renderer& renderer)
{
	m_capture.Update(renderer);
	if (m_capture.HasReadback())
	{
		ViewportCaptureReadback readback = m_capture.TakeReadback();
		if (readback.Result)
		{
			std::string errorMessage;
			m_writeOperation.Start(
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
			    errorMessage);
		}
	}

	ViewportCaptureResult result;
	m_writeOperation.TryConsume(result);
}

std::filesystem::path EditorViewportCaptureCoordinator::BuildOutputPath(std::uint64_t frameId) const
{
	return Filesystem::GetWorkspaceRootPath() / "Saved" / "Captures" / ("Viewport_" + std::to_string(frameId) + ".bmp");
}
