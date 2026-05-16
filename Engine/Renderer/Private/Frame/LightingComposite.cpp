#include "../PCH.h"
#include "Frame/LightingComposite.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/LightingCompositePass.h"

void AddLightingCompositePass(
    FrameGraphBuilder& builder,
    const SceneTargets& sceneTargets,
    const LightingTargets& lighting,
    const GBufferTargets& gbuffer)
{
    auto& parameters = builder.AllocPassParameters<LightingCompositePass>();
    LightingCompositePass::DeclareResources(builder, sceneTargets, lighting, gbuffer, parameters);
    builder.AddComputePass<LightingCompositePass>(
        LightingCompositePass::PassName,
        parameters,
        [](PassExecutionContext& context, LightingCompositePass::ParameterInstance& passParameters)
        {
            const LightingCompositePass pass(context.RuntimeServices.GetPassRuntime<LightingCompositePass>());
            pass.Execute(context, passParameters);
        });
}
