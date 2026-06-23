#include "../../PCH.h"
#include "Frame/Lighting/IndirectDiffuse.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Core/ShaderPass.h"
#include "Passes/Deferred/IndirectDiffusePass.h"

namespace
{
	bool CanUseDescriptorSceneTlas(const FrameContext& frame) noexcept
	{
		return RayTracingScenePassBinding::FrameUsesSceneTlasAccessMode(
		    frame,
		    RayTracingSceneTlasShaderAccessMode::Descriptor);
	}
}

void AddIndirectDiffusePass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameters = builder.AllocPassParameters<IndirectDiffusePass>();
	IndirectDiffusePass::DeclareResources(builder, lighting, gbuffer, sceneTlas, parameters);
	builder.AddPass(
	    IndirectDiffusePass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&parameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!CanUseDescriptorSceneTlas(frame))
		    {
			    return;
		    }

		    ComputeShaderPass<IndirectDiffusePass::Parameters>::Setup(
		        resourceBuilder,
		        parameters,
		        IndirectDiffusePass::PassName);
	    },
	    [&parameters](PassExecutionContext& context)
	    {
		    if (!CanUseDescriptorSceneTlas(context.Frame))
		    {
			    return;
		    }

		    const IndirectDiffusePass pass(context.RuntimeServices.GetPassRuntime<IndirectDiffusePass>());
		    pass.Execute(context, parameters);
	    });
}
