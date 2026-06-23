#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/RayTracing/RayTracingSceneFrameGraphResources.h"

class FrameGraphBuilder;

RayTracingSceneFrameGraphResources CreateRayTracingSceneFrameGraphResources(FrameGraphBuilder& builder);
void AddRayTracingSceneBuildPasses(FrameGraphBuilder& builder, const RayTracingSceneFrameGraphResources& resources);
void AddRayTracingInfrastructurePasses(FrameGraphBuilder& builder, FrameAssemblyResourceLayout& resources);
