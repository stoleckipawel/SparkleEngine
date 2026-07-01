#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct ReferenceRenderTargets
{
	FrameGraphTextureHandle ReferenceDirect;
	FrameGraphTextureHandle ReferenceIndirectDiffuse;
	FrameGraphTextureHandle ReferenceIndirectSpecular;
	FrameGraphTextureHandle ReferenceSceneColor;
};

ReferenceRenderTargets CreateReferenceRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent);
void AddReferenceTargetClearPass(FrameGraphBuilder& builder, const ReferenceRenderTargets& targets);
