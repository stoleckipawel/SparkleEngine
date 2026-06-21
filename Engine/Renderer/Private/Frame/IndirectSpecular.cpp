#include "../PCH.h"
#include "Frame/IndirectSpecular.h"

#include "Frame/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/IndirectSpecularPass.h"
#include "Passes/ShaderPass.h"

void AddIndirectSpecularPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameters = builder.AllocPassParameters<IndirectSpecularPass>();
	IndirectSpecularPass::DeclareResources(builder, lighting, gbuffer, sceneTlas, parameters);
	builder.AddPass(
	    IndirectSpecularPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&parameters](PassResourceBuilder& resourceBuilder, const FrameContext&)
	    {
		    ComputeShaderPass<IndirectSpecularPass::Parameters>::Setup(
		        resourceBuilder,
		        parameters,
		        IndirectSpecularPass::PassName);
	    },
	    [&parameters](PassExecutionContext& context)
	    {
		    const IndirectSpecularPass pass(context.RuntimeServices.GetPassRuntime<IndirectSpecularPass>());
		    pass.Execute(context, parameters);
	    });
}
