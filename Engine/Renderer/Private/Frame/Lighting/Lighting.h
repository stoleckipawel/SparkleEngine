#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources);
