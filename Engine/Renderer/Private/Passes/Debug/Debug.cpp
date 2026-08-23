#include "../../PCH.h"
#include "Passes/Debug/Debug.h"

#include "Passes/Debug/VisualizeBuffers.h"

void AddDebugPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	if (resources.Transient.GBuffer.BaseColor.IsValid() && resources.Transient.Lighting.DirectDiffuse.IsValid())
	{
		AddVisualizeBuffersPass(
		    builder,
		    sceneExtent,
		    resources.ResolvedSceneColor,
		    resources.Transient.Lighting,
		    resources.Transient.GBuffer);
	}
}
