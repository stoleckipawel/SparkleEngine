#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"

struct FrameGraphSceneTargets
{
	TextureHandle SceneColor;
	TextureHandle BackBuffer;
	TextureHandle MainDepth;
};

struct FrameGraphGBufferTargets
{
	TextureHandle BaseColor;
	TextureHandle Normal;
	TextureHandle Material;
	TextureHandle Emissive;
	TextureHandle DeviceZ;
	TextureHandle MainDepth;
};

struct FrameGraphPresentationInputs
{
	TextureHandle BackBuffer;
};

struct FrameGraphComputeShowcaseOutputs
{
	TextureHandle Color;
};
