#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct SceneRenderTargets
{
	FrameGraphTextureHandle SceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SceneDepth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle BackBuffer = FrameGraphTextureHandle::Invalid();
};

struct GBufferRenderTargets
{
	FrameGraphTextureHandle BaseColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Normal = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Material = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Emissive = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Subsurface = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DeviceZ = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVector = FrameGraphTextureHandle::Invalid();
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

		bool IsValid() const noexcept
		{
			return DiffuseAlbedo.IsValid() && SpecularAlbedo.IsValid() && Roughness.IsValid() && SpecularHitDistance.IsValid();
		}
	} ReconstructionGuides;
};
