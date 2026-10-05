#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void CreateDirectLightingResources(FrameGraphBuilder& builder, RenderViewportExtent extent, RenderFrameGraphResources& resources);

void CreateDirectLightReservoirResources(FrameGraphBuilder& builder, RenderViewportExtent extent, RenderFrameGraphResources& resources);
