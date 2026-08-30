#include "PCH.h"

#include "Editor/Capture/EditorViewportCaptureCoordinator.h"

#include "Core/Public/FileSystemUtils.h"
#include "EditorOperations/EditorOperationService.h"
#include "Renderer.h"

EditorViewportCaptureCoordinator::EditorViewportCaptureCoordinator(EditorOperationService& operations) noexcept :
    m_operations(&operations)
{
}

void EditorViewportCaptureCoordinator::Request(Renderer& renderer, std::uint64_t frameId)
{
	if (m_activeCapture)
	{
		return;
	}
	m_activeCapture = renderer.RequestViewportCapture(
	    ViewportCaptureRequest{
	        .Output = RenderOutputFlags::SceneColor,
	        .OutputPath = BuildOutputPath(frameId),
	        .ExpectedFrameId = 0,
	        .ViewModeName = "Editor viewport",
	        .DebugName = "Editor viewport capture"});
}

void EditorViewportCaptureCoordinator::Update(Renderer& renderer)
{
	ViewportCaptureReadback readback;
	while (renderer.TryTakeViewportCapture(readback))
	{
		if (readback.Id.Value != m_activeCapture.Value)
		{
			continue;
		}
		m_activeCapture = {};
		if (!readback.Result)
		{
			m_lastResult = std::move(readback.Result);
			continue;
		}

		std::string errorMessage;
		if (!m_operations->StartViewportCaptureWrite(std::move(readback), errorMessage))
		{
			m_lastResult.Status = ViewportCaptureStatus::Failed;
			m_lastResult.FailureReason = std::move(errorMessage);
		}
	}

	ViewportCaptureResult result;
	if (m_operations->TryConsumeViewportCapture(result))
	{
		m_lastResult = std::move(result);
	}
}

std::filesystem::path EditorViewportCaptureCoordinator::BuildOutputPath(std::uint64_t frameId) const
{
	return Filesystem::GetWorkspaceRootPath() / "Saved" / "Captures" / ("Viewport_" + std::to_string(frameId) + ".bmp");
}
