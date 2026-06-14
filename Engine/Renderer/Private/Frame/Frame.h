#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Frame/RayTracingSceneFrameGraphResources.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct FrameBuildResult
{
	SceneRenderTargets Scene;
	GBufferRenderTargets GBuffer;
	RayTracingSceneFrameGraphResources RayTracing;
};

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, bool presentToBackBuffer);
