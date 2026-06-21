#pragma once

#include "Frame/RayTracing/RayTracingSceneFrameGraphResources.h"

class FrameGraphBuilder;

RayTracingSceneFrameGraphResources CreateRayTracingSceneFrameGraphResources(FrameGraphBuilder& builder);
void AddRayTracingSceneBuildPasses(FrameGraphBuilder& builder, const RayTracingSceneFrameGraphResources& resources);
