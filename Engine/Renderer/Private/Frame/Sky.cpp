#include "../PCH.h"
#include "Frame/Sky.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/SkyPass.h"

void AddSkyPass(FrameGraphBuilder& builder, const SceneTargets& sceneTargets, const GBufferTargets& gbuffer)
{
	auto& parameters = builder.AllocPassParameters<SkyPass>();
	SkyPass::DeclareResources(builder, sceneTargets, gbuffer, parameters);
	builder.AddComputePass<SkyPass>(
	    SkyPass::PassName,
	    parameters,
	    [](PassExecutionContext& context, SkyPass::ParameterInstance& passParameters)
	    {
		    const SkyPass pass(context.RuntimeServices.GetPassRuntime<SkyPass>());
		    pass.Execute(context, passParameters);
	    });
}
