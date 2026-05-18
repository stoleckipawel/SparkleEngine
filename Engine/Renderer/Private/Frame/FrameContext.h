#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Frame/RenderViewData.h"
#include "Frame/MeshInstanceFrameData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

struct SPARKLE_RENDERER_API FrameContext
{
	RenderSceneData sceneData = {};
	RenderViewData mainView = {};
	MeshInstanceFrameData meshInstances = {};
};
