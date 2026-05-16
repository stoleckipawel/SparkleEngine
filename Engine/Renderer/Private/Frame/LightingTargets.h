#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

LightingTargets CreateLightingTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent);
