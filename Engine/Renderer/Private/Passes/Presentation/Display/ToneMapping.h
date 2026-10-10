#pragma once

struct RenderFrame;

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

FrameGraphTextureHandle AddToneMappingPass(FrameGraphBuilder& builder, const RenderFrame& frame, RenderViewportExtent outputExtent, const RenderFrameGraphResources& resources);
