#pragma once

#include "Editor/Public/EditorAPI.h"
#include "Editor/Public/Viewport/ViewportOverlay.h"
#include "RHI/Public/Diagnostics/RhiExternalCapture.h"
#include <memory>

class EditorExternalCaptureCommands
{
public:
	virtual ~EditorExternalCaptureCommands() noexcept = default;

	virtual ExternalCaptureSnapshot Observe() const = 0;
	virtual ExternalCaptureAdmission Request() noexcept = 0;
};

SPARKLE_EDITOR_API std::unique_ptr<ViewportOverlay> CreateExternalCaptureOverlay(std::unique_ptr<EditorExternalCaptureCommands> commands);
