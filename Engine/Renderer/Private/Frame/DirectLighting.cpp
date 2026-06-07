#include "../PCH.h"
#include "Frame/DirectLighting.h"

#include "Frame/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/DirectLightingPass.h"
#include "Passes/ShaderPass.h"

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameterStorage = builder.AllocPassParameters<DirectLightingPass>();
	auto* parameters = &parameterStorage;
	auto setupValid = std::make_shared<bool>(true);
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, *parameters);

	builder.AddPass(
	    DirectLightingPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [sceneTlas, parameters, setupValid](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (sceneTlas.IsValid() && frame.rayTracingScene.HasBoundTlas())
		    {
			    resourceBuilder.Read(sceneTlas, ResourceUsage::AccelerationStructureRead);
		    }

		    *setupValid = ComputeShaderPass<DirectLightingPass::Parameters>::Setup(resourceBuilder, *parameters, DirectLightingPass::PassName);
	    },
	    [parameters, setupValid](PassExecutionContext& context)
	    {
		    if (!*setupValid)
		    {
			    return;
		    }

		    const DirectLightingPass pass(context.RuntimeServices.GetPassRuntime<DirectLightingPass>());
		    pass.Execute(context, *parameters);
	    });
}
