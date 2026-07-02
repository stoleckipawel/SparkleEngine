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
	FrameGraphTextureHandle ReferencePrimaryDeviceDepth;
	FrameGraphTextureHandle ReferencePrimaryNormal;
	FrameGraphTextureHandle ReferencePrimaryDiffuseAlbedo;
	FrameGraphTextureHandle ReferencePrimarySpecularAlbedo;
	FrameGraphTextureHandle ReferencePrimaryMaterialGuide;
	FrameGraphTextureHandle ReferencePrimaryPathSampleGuide;
};

ReferenceRenderTargets CreateReferenceRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent);
void AddReferenceTargetClearPass(FrameGraphBuilder& builder, const ReferenceRenderTargets& targets);
