#pragma once

#include "RayTracing/Scene/RayTracingSceneFrameData.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "SceneData/RenderSceneGpuData.h"
#include "View/RenderView.h"

struct FrameContext
{
	PreparedRenderScene preparedScene = {};
	const RenderSceneGpuData* sceneGpuData = nullptr;
	RayTracingSceneFrameData rayTracingScene = {};
	RenderView view = {};
};
