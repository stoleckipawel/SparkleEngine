#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct SceneRenderTargets
{
	FrameGraphTextureHandle SceneColor;
	FrameGraphTextureHandle SceneDepth;
	FrameGraphTextureHandle FinalSceneColor;
	FrameGraphTextureHandle BackBuffer;
};

struct GBufferRenderTargets
{
	FrameGraphTextureHandle BaseColor;
	FrameGraphTextureHandle Normal;
	FrameGraphTextureHandle Material;
	FrameGraphTextureHandle Emissive;
	FrameGraphTextureHandle Subsurface;
	FrameGraphTextureHandle DeviceZ;
	FrameGraphTextureHandle MotionVector;
};

struct LightingRenderTargets
{
	FrameGraphTextureHandle DirectDiffuse;
	FrameGraphTextureHandle DirectSpecular;
	FrameGraphTextureHandle DirectSubsurface;
	FrameGraphTextureHandle IndirectDiffuse;
	FrameGraphTextureHandle IndirectSpecular;

	struct RayReconstructionGuides final
	{
		FrameGraphTextureHandle DiffuseAlbedo = FrameGraphTextureHandle::Invalid();
		FrameGraphTextureHandle SpecularAlbedo = FrameGraphTextureHandle::Invalid();
		FrameGraphTextureHandle Roughness = FrameGraphTextureHandle::Invalid();
		FrameGraphTextureHandle SpecularHitDistance = FrameGraphTextureHandle::Invalid();

		bool IsValid() const noexcept
		{
			return DiffuseAlbedo.IsValid() && SpecularAlbedo.IsValid() && Roughness.IsValid() &&
			       SpecularHitDistance.IsValid();
		}
	} ReconstructionGuides;
};
