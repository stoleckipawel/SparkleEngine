#include "../PCH.h"
#include "Frame/IndirectLighting.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/IndirectLightingPass.h"

void AddIndirectLightingPass(FrameGraphBuilder& builder, const LightingTargets& lighting)
{
	auto& parameters = builder.AllocPassParameters<IndirectLightingPass>();
	IndirectLightingPass::DeclareResources(builder, lighting, parameters);
	builder.AddComputePass<IndirectLightingPass>(
	    IndirectLightingPass::PassName,
	    parameters,
	    [](PassExecutionContext& context, IndirectLightingPass::ParameterInstance& passParameters)
	    {
		    const IndirectLightingPass pass(context.RuntimeServices.GetPassRuntime<IndirectLightingPass>());
		    pass.Execute(context, passParameters);
	    });
}
