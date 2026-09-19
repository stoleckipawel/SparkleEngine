#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirLightingResources.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Resources/History/FrameHistory.h"

void CreateRestirLightingResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	DeclareRestirLightingHistoryResources(builder, sceneExtent, resources.History);
}
