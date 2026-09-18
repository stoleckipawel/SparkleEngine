#pragma once

#include "EditorOperations/EditorOperationSlot.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>
#include <filesystem>

class EditorOperationRuntime;
class Renderer;

class EditorViewportCaptureCoordinator final
{
public:
	explicit EditorViewportCaptureCoordinator(EditorOperationRuntime& operations) noexcept;

	void Request(Renderer& renderer, std::uint64_t frameId);
	void Update(Renderer& renderer);

private:
	std::filesystem::path BuildOutputPath(std::uint64_t frameId) const;

	EditorOperationSlot<ViewportCaptureResult> m_writeOperation;
	ViewportCaptureId m_activeCapture;
	std::filesystem::path m_outputPath;
	ViewportCaptureResult m_lastResult;
};
