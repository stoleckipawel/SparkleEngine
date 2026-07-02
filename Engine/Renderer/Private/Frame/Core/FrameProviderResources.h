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

struct FrameIndirectReconstructionProviderResources final
{
	FrameGraphTextureHandle NoisyIndirectDiffuse = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle NoisyIndirectSpecular = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DemodulatedIndirectDiffuse = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DemodulatedIndirectSpecular = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DiffuseAlbedo = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SpecularAlbedo = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MaterialGuide = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DiffuseSampleGuide = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SpecularSampleGuide = FrameGraphTextureHandle::Invalid();
};

struct FrameRayReconstructionProviderResources final
{
	FrameGraphTextureHandle NoisyInputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle OutputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Depth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVectors = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Normals = FrameGraphTextureHandle::Invalid();
	FrameIndirectReconstructionProviderResources IndirectReconstruction = {};
};
