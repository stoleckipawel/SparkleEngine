#pragma once

#include "Editor/Public/Viewport/ViewportOverlay.h"
#include <memory>
class Renderer;
std::unique_ptr<ViewportOverlay> CreateEditorExternalCaptureOverlay(Renderer& renderer);
