#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"

namespace FrameGraphCompilerRayTracing
{
	bool UsesRayTracingState(const PassResourceDeclaration& declaration) noexcept;
	ResourceState InferRequiredResourceState(const PassResourceDeclaration& declaration, const FrameGraphResourceNode& resource) noexcept;
	bool RequiresExecutionBarrier(const PassResourceDeclaration& declaration, ResourceState currentState, ResourceState requiredState) noexcept;
	FrameGraphBarrier BuildExecutionBarrier(
	    FrameGraphResourceHandle handle,
	    const PassResourceDeclaration& declaration,
	    ResourceState before,
	    ResourceState after) noexcept;
}
