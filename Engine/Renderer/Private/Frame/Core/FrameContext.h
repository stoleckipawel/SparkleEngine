#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Frame/Core/RenderViewData.h"
#include "RayTracing/Scene/RayTracingSceneFrameData.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/RenderSceneGpuData.h"

struct SPARKLE_RENDERER_API FrameContext
{
	RenderSceneData sceneData = {};
	RenderSceneGpuData sceneGpuData = {};
	RayTracingSceneFrameData rayTracingScene = {};
	RenderViewData mainView = {};
};
