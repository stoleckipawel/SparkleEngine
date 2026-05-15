#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct SceneTargets
{
	FrameGraphTextureHandle SceneColor;
	FrameGraphTextureHandle BackBuffer;
	FrameGraphTextureHandle MainDepth;
};

struct GBufferTargets
{
	FrameGraphTextureHandle BaseColor;
	FrameGraphTextureHandle Normal;
	FrameGraphTextureHandle Material;
	FrameGraphTextureHandle Emissive;
	FrameGraphTextureHandle Subsurface;
	FrameGraphTextureHandle DeviceZ;
	FrameGraphTextureHandle MainDepth;
};

struct LightingTargets
{
	FrameGraphTextureHandle DirectDiffuse;
	FrameGraphTextureHandle DirectSpecular;
	FrameGraphTextureHandle DirectSubsurface;
	FrameGraphTextureHandle IndirectDiffuse;
	FrameGraphTextureHandle IndirectSpecular;
	FrameGraphTextureHandle IndirectSubsurface;
};
