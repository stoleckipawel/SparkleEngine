#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraph;

LightingTargets CreateLightingTargets(FrameGraph& frameGraph, RenderViewportExtent sceneExtent);