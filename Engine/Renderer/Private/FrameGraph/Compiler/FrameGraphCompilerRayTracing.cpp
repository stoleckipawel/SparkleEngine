#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphCompilerRayTracing.h"

#include <cassert>

bool FrameGraphCompilerRayTracing::UsesRayTracingState(const PassResourceDeclaration& declaration) noexcept
{
	return UsesAccelerationStructure(declaration.usage);
}

ResourceState FrameGraphCompilerRayTracing::InferRequiredResourceState(
    const PassResourceDeclaration& declaration,
    const FrameGraphResourceNode& resource) noexcept
{
	assert(UsesRayTracingState(declaration));
	assert(resource.kind == FrameGraphResourceKind::AccelerationStructure);
	return ResourceState::RayTracingAccelerationStructure;
}

bool FrameGraphCompilerRayTracing::RequiresTransitionBarrier(ResourceState currentState, ResourceState requiredState) noexcept
{
	return currentState != requiredState;
}
