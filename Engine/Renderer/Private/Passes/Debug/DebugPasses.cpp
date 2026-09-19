#include "../../PCH.h"
#include "Passes/Debug/DebugPasses.h"

#include "Passes/Debug/VisualizeBuffers.h"

void AddDebugPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	AddVisualizeBuffersPass(builder, sceneExtent, resources);
}
