#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

void AddRestirLightingProducerPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources);
void FinalizeRestirLightingPasses(FrameAssemblyResourceLayout& resources);
