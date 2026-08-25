#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;
class RenderRayTracingScene;

FrameGraphAccelerationStructureHandle CreateRayTracingSceneFrameGraphResource(FrameGraphBuilder& builder);
void AddRayTracingSceneBuildPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    FrameGraphAccelerationStructureHandle sceneTlas);
void AddRayTracingScenePasses(FrameGraphBuilder& builder, RenderRayTracingScene& rayTracingScene, RenderFrameGraphResources& resources);
