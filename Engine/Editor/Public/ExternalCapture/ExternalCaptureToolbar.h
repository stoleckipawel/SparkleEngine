#pragma once

#include "Editor/Public/EditorAPI.h"
#include "Editor/Public/Viewport/ViewportToolbarActions.h"

#include <memory>

// Launch requests are distinct from native attachment and capture readiness.
struct ExternalCaptureToolRequests final
{
	bool Pix = false;
	bool Nsight = false;
	bool RenderDoc = false;
};

// Creates only the optional capture action group; does not access ImGui or load a tool.
SPARKLE_EDITOR_API std::unique_ptr<ViewportToolbarActions> CreateExternalCaptureToolbarActions(ExternalCaptureToolRequests requested);
