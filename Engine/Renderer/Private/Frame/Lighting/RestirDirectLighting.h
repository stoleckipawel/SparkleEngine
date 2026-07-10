#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddRestirDirectLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources);
