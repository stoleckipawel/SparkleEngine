#include "../../PCH.h"
#include "Frame/Debug/Debug.h"

#include "Frame/Debug/VisualizeBuffers.h"

void AddDebugPasses(
    FrameGraphBuilder& builder,
    const FrameAssemblyResourceLayout& resources)
{
	if (resources.Transient.GBuffer.BaseColor.IsValid() && resources.Transient.Lighting.DirectDiffuse.IsValid())
	{
		AddVisualizeBuffersPass(builder, resources.Transient.Scene, resources.Transient.Lighting, resources.Transient.GBuffer);
	}
}
