#include "../PCH.h"
#include "Frame/DirectLighting.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/DirectLightingPass.h"

void AddDirectLightingPass(FrameGraphBuilder& builder, const LightingTargets& lighting, const GBufferTargets& gbuffer)
{
	auto& parameters = builder.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, parameters);

	builder.AddComputePass<DirectLightingPass>(
	    DirectLightingPass::PassName,
	    parameters,
	    [](PassExecutionContext& context, DirectLightingPass::ParameterInstance& passParameters)
	    {
		    const DirectLightingPass pass(context.RuntimeServices.GetPassRuntime<DirectLightingPass>());
		    pass.Execute(context, passParameters);
	    });
}
