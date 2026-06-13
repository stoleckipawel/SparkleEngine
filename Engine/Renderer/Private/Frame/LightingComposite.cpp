#include "../PCH.h"
#include "Frame/LightingComposite.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/LightingCompositePass.h"

void AddLightingCompositePass(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer)
{
	auto& parameters = builder.AllocPassParameters<LightingCompositePass>();
	LightingCompositePass::DeclareResources(builder, sceneTargets, lighting, gbuffer, parameters);
	builder.AddComputeShaderPass<LightingCompositePass>(parameters);
}
