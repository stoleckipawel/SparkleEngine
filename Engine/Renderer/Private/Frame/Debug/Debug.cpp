#include "../../PCH.h"
#include "Frame/Debug/Debug.h"

#include "Frame/Debug/VisualizeBuffers.h"

void AddDebugPasses(
    FrameGraphBuilder& builder,
    const FrameAssemblyResourceLayout& resources)
{
	AddVisualizeBuffersPass(builder, resources.Transient.Scene, resources.Transient.Lighting, resources.Transient.GBuffer);
}
