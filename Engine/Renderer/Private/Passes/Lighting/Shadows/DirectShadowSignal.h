#pragma once

struct RenderFrame;

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RenderRayTracingScene;
struct RenderFrameGraphResources;

void AddDirectShadowSignalPass(FrameGraphBuilder& builder, const RenderFrame& frame, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources, RenderRayTracingScene& rayTracingScene);
