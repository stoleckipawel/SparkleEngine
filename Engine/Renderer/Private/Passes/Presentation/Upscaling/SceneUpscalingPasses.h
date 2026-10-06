#pragma once

#include "Passes/Presentation/PresentationPolicy.h"

class FrameGraphBuilder;
class RendererImageProviderStack;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void AddSceneUpscalingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    SceneUpscalingMethod method,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources);
