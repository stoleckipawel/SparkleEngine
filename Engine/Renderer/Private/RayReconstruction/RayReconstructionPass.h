#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct RayReconstructionPassResources final
{
	FrameGraphTextureHandle NoisyInputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle OutputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Depth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVectors = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Normals = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Roughness = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DiffuseAlbedo = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SpecularAlbedo = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SpecularHitDistance = FrameGraphTextureHandle::Invalid();
};

void AddRayReconstructionPass(
    FrameGraphBuilder& builder,
    const char* passName,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    const RayReconstructionPassResources& inputs);
