#pragma once

#include "Frame/Core/RenderViewData.h"
#include "RayTracing/Scene/RayTracingSceneFrameData.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/RenderSceneGpuData.h"

struct FrameContext
{
	RenderSceneData sceneData = {};
	RenderSceneGpuData sceneGpuData = {};
	RayTracingSceneFrameData rayTracingScene = {};
	RenderViewData mainView = {};
};
