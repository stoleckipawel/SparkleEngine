#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphPlan.h"

void FrameGraphTransientLifetime::Clear() noexcept
{
	firstUserPass = INVALID_FRAME_GRAPH_PASS_INDEX;
	lastUserPass = INVALID_FRAME_GRAPH_PASS_INDEX;
	firstExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
	lastExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
	readUsed = false;
	writeUsed = false;
	requiredStates.clear();
}

void FrameGraphTransientPlan::Clear() noexcept
{
	resources.clear();
	physicalBlocks.clear();
}

void FrameGraphPlan::Clear() noexcept
{
	passes.clear();
	resources.clear();
	productRoots.clear();
	transients.Clear();
	executionOrder.clear();
	initialTransientAliasingBarriers.clear();
	initialBarriers.clear();
	finalBarriers.clear();
	submissionBatches.clear();
	recording.Clear();
}
