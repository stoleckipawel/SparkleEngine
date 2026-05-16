#include "../PCH.h"
#include "Frame/VisualizeBuffers.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/VisualizeBuffersPass.h"

void AddVisualizeBuffersPass(
    FrameGraphBuilder& builder,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer)
{
	auto& parameters = builder.AllocPassParameters<VisualizeBuffersPass>();
	VisualizeBuffersPass::DeclareResources(builder, sceneTargets, lighting, gbuffer, parameters);
	builder.AddComputePass<VisualizeBuffersPass>(VisualizeBuffersPass::PassName, parameters);
}
