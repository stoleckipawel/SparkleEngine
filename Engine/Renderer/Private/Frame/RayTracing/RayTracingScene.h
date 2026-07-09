#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;

FrameGraphAccelerationStructureHandle CreateRayTracingSceneFrameGraphResource(FrameGraphBuilder& builder);
void AddRayTracingSceneBuildPasses(FrameGraphBuilder& builder, FrameGraphAccelerationStructureHandle sceneTlas);
void AddRaytracingScenePasses(FrameGraphBuilder& builder, FrameAssemblyResourceLayout& resources);
