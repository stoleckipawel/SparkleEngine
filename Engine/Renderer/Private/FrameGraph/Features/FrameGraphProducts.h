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
	TextureHandle DeviceZ;
	TextureHandle MainDepth;
};
