#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "FrameGraph/FrameGraphResourceRegistry.h"
#include "FrameGraph/FrameGraphResourceStateTracker.h"

namespace FrameGraphCompilerExternalResources
{
	void ValidateResourceBoundaryState(
	    const FrameGraphResourceMetadata& metadata,
	    const FrameGraphResourceRuntimeState& runtimeState) noexcept;

	bool ShouldRestoreFinalState(const FrameGraphResourceNode& resource) noexcept;
}
