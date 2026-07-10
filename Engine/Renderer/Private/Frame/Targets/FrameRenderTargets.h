#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct SceneRenderTargets
{
	FrameGraphTextureHandle SceneColor;
	FrameGraphTextureHandle SceneDepth;
	FrameGraphTextureHandle ReconstructedSceneColor;
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
	FrameGraphTextureHandle IndirectDiffuseAlbedo;
	FrameGraphTextureHandle IndirectSpecularAlbedo;
	FrameGraphTextureHandle IndirectMaterialGuide;
	FrameGraphTextureHandle IndirectSpecularSampleGuide;
};
