#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;
struct RenderView;

class ReferencePathTracer final
{
public:
	static void AddPass(FrameGraphBuilder& builder, RenderViewportExtent extent, const RenderFrameGraphResources& resources);

	ViewportRenderProgress Update(const RenderView& view) const noexcept;
};
