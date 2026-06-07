#include "../PCH.h"
#include "Frame/DirectLighting.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/DirectLightingPass.h"

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameters = builder.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, sceneTlas, parameters);

	builder.AddComputePass<DirectLightingPass>(
	    DirectLightingPass::PassName,
	    parameters,
	    [](PassExecutionContext& context, DirectLightingPass::ParameterInstance& passParameters)
	    {
		    const DirectLightingPass pass(context.RuntimeServices.GetPassRuntime<DirectLightingPass>());
		    pass.Execute(context, passParameters);
	    });
}
