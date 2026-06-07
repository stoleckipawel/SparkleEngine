#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;

void AddRayTracingSceneBuildPass(FrameGraphBuilder& builder, FrameGraphAccelerationStructureHandle sceneTlas);
