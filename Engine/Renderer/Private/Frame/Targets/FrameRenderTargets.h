#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct SceneRenderTargets
{
	FrameGraphTextureHandle SceneColor;
	FrameGraphTextureHandle FinalSceneColor;
	FrameGraphTextureHandle BackBuffer;
	FrameGraphTextureHandle MainDepth;
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
	FrameGraphTextureHandle MainDepth;
};

struct LightingRenderTargets
{
	FrameGraphTextureHandle DirectDiffuse;
	FrameGraphTextureHandle DirectSpecular;
	FrameGraphTextureHandle DirectSubsurface;
	FrameGraphTextureHandle IndirectDiffuse;
	FrameGraphTextureHandle IndirectSpecular;
	FrameGraphTextureHandle IndirectSubsurface;
	FrameGraphTextureHandle IndirectDiffuseDemodulatedRadiance;
	FrameGraphTextureHandle IndirectSpecularDemodulatedRadiance;
	FrameGraphTextureHandle IndirectDiffuseAlbedo;
	FrameGraphTextureHandle IndirectSpecularAlbedo;
	FrameGraphTextureHandle IndirectMaterialGuide;
	FrameGraphTextureHandle IndirectDiffuseSampleGuide;
	FrameGraphTextureHandle IndirectSpecularSampleGuide;
};
