#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"

namespace FrameGraphCompilerRayTracing
{
	bool UsesRayTracingState(const PassResourceDeclaration& declaration) noexcept;
	ResourceState InferRequiredResourceState(const PassResourceDeclaration& declaration, const FrameGraphResourceNode& resource) noexcept;
	bool RequiresTransitionBarrier(ResourceState currentState, ResourceState requiredState) noexcept;
}
