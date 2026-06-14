#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Frame/RenderViewData.h"
#include "Frame/MeshInstanceFrameData.h"
#include "Frame/RayTracingSceneFrameData.h"
#include "Frame/SkinningFrameData.h"
#include "RayTracing/RayTracingSceneFramePlan.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

struct SPARKLE_RENDERER_API FrameContext
{
	RenderSceneData sceneData = {};
	RayTracingSceneFramePlan rayTracingFramePlan = {};
	RayTracingSceneFrameData rayTracingScene = {};
	RenderViewData mainView = {};
	MeshInstanceFrameData meshInstances = {};
	SkinningFrameData skinning = {};
};
