#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RenderRayTracingScene;

void AddRestirLightingProducerPasses(
	FrameGraphBuilder& builder,
	RenderRayTracingScene& rayTracingScene,
	RenderViewportExtent sceneExtent,
	RenderFrameGraphResources& resources);
