#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct FrameUpscalerProviderResources final
{
	FrameGraphTextureHandle ScalingInputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle ScalingOutputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Depth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVectors = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
};

struct FrameRayReconstructionProviderResources final
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
