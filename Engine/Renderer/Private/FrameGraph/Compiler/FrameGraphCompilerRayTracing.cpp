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

bool FrameGraphCompilerRayTracing::RequiresExecutionBarrier(
    const PassResourceDeclaration& declaration,
    ResourceState currentState,
    ResourceState requiredState) noexcept
{
	if (currentState != requiredState)
	{
		return true;
	}

	return declaration.usage == ResourceUsage::AccelerationStructureBuild;
}

FrameGraphBarrier FrameGraphCompilerRayTracing::BuildExecutionBarrier(
    FrameGraphResourceHandle handle,
    const PassResourceDeclaration& declaration,
    ResourceState before,
    ResourceState after) noexcept
{
	if (before != after)
	{
		return FrameGraphBarrier{
		    .handle = handle,
		    .type = FrameGraphBarrier::Type::Transition,
		    .before = before,
		    .after = after,
		    .label = declaration.label};
	}

	assert(declaration.usage == ResourceUsage::AccelerationStructureBuild);
	return FrameGraphBarrier{
	    .handle = handle,
	    .type = FrameGraphBarrier::Type::AccelerationStructure,
	    .before = before,
	    .after = after,
	    .label = declaration.label};
}
