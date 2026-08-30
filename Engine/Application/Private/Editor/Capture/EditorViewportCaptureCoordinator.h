#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class EditorOperationService;
class Renderer;

class EditorViewportCaptureCoordinator final
{
public:
	explicit EditorViewportCaptureCoordinator(EditorOperationService& operations) noexcept;

	void Request(Renderer& renderer, std::uint64_t frameId);
	void Update(Renderer& renderer);

private:
	std::filesystem::path BuildOutputPath(std::uint64_t frameId) const;

	EditorOperationService* m_operations = nullptr;
	ViewportCaptureId m_activeCapture;
	ViewportCaptureResult m_lastResult;
};
