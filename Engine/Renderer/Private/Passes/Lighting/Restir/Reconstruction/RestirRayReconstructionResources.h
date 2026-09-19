#pragma once

#include "RayReconstruction/RayReconstructionPass.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

RayReconstructionPassResources CreateRestirRayReconstructionResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    const RenderFrameGraphResources& resources);
