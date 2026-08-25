#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphCompilerRayTracing.h"

#include <cassert>

bool FrameGraphCompilerRayTracing::UsesRayTracingState(const PassResourceDeclaration& declaration) noexcept
{
	return UsesAccelerationStructure(declaration.usage) || UsesRayTracingShaderTable(declaration.usage);
}

ResourceState FrameGraphCompilerRayTracing::InferRequiredResourceState(
    const PassResourceDeclaration& declaration,
    const FrameGraphResourceNode& resource) noexcept
{
	assert(UsesRayTracingState(declaration));
	if (UsesAccelerationStructure(declaration.usage))
	{
		assert(resource.kind == FrameGraphResourceKind::AccelerationStructure);
		return ResourceState::RayTracingAccelerationStructure;
	}
	assert(resource.kind == FrameGraphResourceKind::Buffer);
	return ResourceState::RayTracingShaderTable;
}

bool FrameGraphCompilerRayTracing::RequiresTransitionBarrier(ResourceState currentState, ResourceState requiredState) noexcept
{
	return currentState != requiredState;
}
