#pragma once

#include "RayReconstruction/RayReconstructionPass.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void CreateRayReconstructionGuideRenderTargets(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderFrameGraphResources& resources);

RayReconstructionPassResources CreateRestirRayReconstructionResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    const RenderFrameGraphResources& resources);
