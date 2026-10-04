#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"

struct SceneRenderTargets
{
	FrameGraphTextureHandle SceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SceneDepth = FrameGraphTextureHandle::Invalid();
};

struct GBufferRenderTargets
{
	FrameGraphTextureHandle BaseColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle WorldNormal = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle WorldTangent = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Material = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Emissive = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Subsurface = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVector = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DeviceZ = FrameGraphTextureHandle::Invalid();
};

struct LightingRenderTargets
{
	FrameGraphTextureHandle DirectDiffuse = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DirectSpecular = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DirectSubsurface = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle IndirectDiffuse = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle IndirectSpecular = FrameGraphTextureHandle::Invalid();

	struct RayReconstructionGuides final
	{
		FrameGraphTextureHandle DiffuseAlbedo = FrameGraphTextureHandle::Invalid();
		FrameGraphTextureHandle SpecularAlbedo = FrameGraphTextureHandle::Invalid();
		FrameGraphTextureHandle Roughness = FrameGraphTextureHandle::Invalid();
		FrameGraphTextureHandle SpecularHitDistance = FrameGraphTextureHandle::Invalid();
	} ReconstructionGuides;
};
