#include "../PCH.h"
#include "Frame/VisualizeBuffers.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/VisualizeBuffersPass.h"

void AddVisualizeBuffersPass(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer)
{
    auto& parameters = builder.AllocPassParameters<VisualizeBuffersPass>();
    VisualizeBuffersPass::DeclareResources(builder, sceneTargets, lighting, gbuffer, parameters);
    builder.AddComputePass<VisualizeBuffersPass>(
        VisualizeBuffersPass::PassName,
        parameters,
        [](PassExecutionContext& context, VisualizeBuffersPass::ParameterInstance& passParameters)
        {
            const VisualizeBuffersPass pass(context.RuntimeServices.GetPassRuntime<VisualizeBuffersPass>());
            pass.Execute(context, passParameters);
        });
}
