#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Frame/RenderViewContext.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

struct SPARKLE_RENDERER_API FrameContext
{
	RenderSceneData sceneData = {};
	RenderViewContext mainView = {};
};
