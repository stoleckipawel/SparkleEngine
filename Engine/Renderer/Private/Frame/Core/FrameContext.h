#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Geometry/MeshInstanceFrameData.h"
#include "Frame/Lighting/FrameLightingData.h"
#include "Frame/Lighting/LightingHistoryContinuity.h"
#include "Frame/RayTracing/RayTracingSceneFrameData.h"
#include "Frame/RayTracing/RayTracingHitDataFrameData.h"
#include "Frame/Geometry/SkinningFrameData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

struct SPARKLE_RENDERER_API FrameContext
{
	RenderSceneData sceneData = {};
	RayTracingSceneFrameData rayTracingScene = {};
	RenderViewData mainView = {};
	FrameLightingData lighting = {};
	LightingHistoryContinuity lightingHistory = {};
	MeshInstanceFrameData meshInstances = {};
	RayTracingHitDataFrameData rayTracingHitData = {};
	SkinningFrameData skinning = {};
};
