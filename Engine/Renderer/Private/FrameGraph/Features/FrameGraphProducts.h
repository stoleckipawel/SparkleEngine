#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"

struct SceneTargets
{
	TextureHandle SceneColor;
	TextureHandle BackBuffer;
	TextureHandle MainDepth;
};

struct GBufferTargets
{
	TextureHandle BaseColor;
	TextureHandle Normal;
	TextureHandle Material;
	TextureHandle Emissive;
	TextureHandle Subsurface;
	TextureHandle DeviceZ;
	TextureHandle MainDepth;
};

struct LightingTargets
{
	TextureHandle DirectDiffuse;
	TextureHandle DirectSpecular;
	TextureHandle DirectSubsurface;
	TextureHandle IndirectDiffuse;
	TextureHandle IndirectSpecular;
	TextureHandle IndirectSubsurface;
};
