#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/Frame/RenderViewContext.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"

struct SPARKLE_RENDERER_API FrameContext
{
	RenderSceneData sceneData = {};
	RenderViewContext mainView = {};
	RenderViewContext shadowView = {};
};
