#include "../PCH.h"
#include "Frame/LightingComposite.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/LightingCompositePass.h"

void AddLightingCompositePass(
    FrameGraphBuilder& builder,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer)
{
	auto& parameters = builder.AllocPassParameters<LightingCompositePass>();
	LightingCompositePass::DeclareResources(builder, sceneTargets, lighting, gbuffer, parameters);
	builder.AddComputePass<LightingCompositePass>(LightingCompositePass::PassName, parameters);
}
