#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;
class RenderRayTracingScene;

FrameGraphAccelerationStructureHandle CreateRayTracingSceneFrameGraphResource(FrameGraphBuilder& builder);
void AddRayTracingSceneBuildPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    FrameGraphAccelerationStructureHandle sceneTlas);
void AddRaytracingScenePasses(FrameGraphBuilder& builder, RenderRayTracingScene& rayTracingScene, FrameAssemblyResourceLayout& resources);
