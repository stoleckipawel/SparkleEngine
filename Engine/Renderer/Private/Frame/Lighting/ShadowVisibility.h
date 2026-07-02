#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;

struct DirectShadowSignalResources final
{
	FrameGraphTextureHandle Visibility = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle LightSample = FrameGraphTextureHandle::Invalid();
};

DirectShadowSignalResources CreateDirectShadowSignalResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources);
