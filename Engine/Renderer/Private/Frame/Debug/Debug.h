#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;

void AddDebugPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const FrameAssemblyResourceLayout& resources);
