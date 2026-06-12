#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphCompilerExternalResources.h"

#include "RHI/Public/Validation/RhiValidation.h"

#include <format>
#include <string>

void FrameGraphCompilerExternalResources::ValidateResourceBoundaryState(
    const FrameGraphResourceMetadata& metadata,
    const FrameGraphResourceRuntimeState& runtimeState) noexcept
{
	if (!IsExternalFrameGraphResource(metadata.ownership) || runtimeState.currentState == metadata.initialState)
	{
		return;
	}

	const std::string condition = std::format(
	    "resource '{}' declared initialState='{}' but tracked runtime state is '{}'",
	    metadata.debugName.empty() ? "<unnamed>" : metadata.debugName,
	    ResourceStateToString(metadata.initialState),
	    ResourceStateToString(runtimeState.currentState));
	RhiValidation::ReportContractViolation(
	    "Renderer.FrameGraph",
	    condition,
	    "update the import/persistent boundary state at the host handoff or transition the resource through FrameGraph before declaring pass usage");
}

bool FrameGraphCompilerExternalResources::ShouldRestoreFinalState(const FrameGraphResourceNode& resource) noexcept
{
	if (resource.finalState == ResourceState::Undefined)
	{
		return false;
	}

	return resource.ownership != FrameGraphResourceOwnership::Transient || resource.kind == FrameGraphResourceKind::DepthStencil;
}
