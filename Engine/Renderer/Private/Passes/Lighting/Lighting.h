#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class IRayReconstructionProvider;

void AddLightingPasses(
    FrameGraphBuilder& builder,
    LightingMode mode,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources);
void AddLightingReconstructionPasses(
    FrameGraphBuilder& builder,
    LightingMode mode,
    RenderViewportExtent sceneExtent,
    RenderViewportExtent outputExtent,
    IRayReconstructionProvider* rayReconstructionProvider,
    RenderFrameGraphResources& resources);
