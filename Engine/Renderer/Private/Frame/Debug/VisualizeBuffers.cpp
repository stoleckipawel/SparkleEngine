#include "../../PCH.h"
#include "Frame/Debug/VisualizeBuffers.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Debug/VisualizeBuffersPass.h"

void AddVisualizeBuffersPass(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer)
{
	auto& parameters = builder.AllocPassParameters<VisualizeBuffersPass>();
	VisualizeBuffersPass::DeclareResources(builder, sceneTargets, lighting, gbuffer, parameters);
	builder.AddComputeShaderPass<VisualizeBuffersPass>(parameters);
}
