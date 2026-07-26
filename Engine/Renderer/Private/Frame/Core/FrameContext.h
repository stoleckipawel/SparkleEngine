#pragma once

#include "Frame/Core/RenderViewData.h"
#include "RayTracing/Scene/RayTracingSceneFrameData.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/RenderSceneGpuData.h"

struct FrameContext
{
	RenderSceneData sceneData = {};
	const RenderSceneGpuData* sceneGpuData = nullptr;
	RayTracingSceneFrameData rayTracingScene = {};
	RenderViewData mainView = {};
};
