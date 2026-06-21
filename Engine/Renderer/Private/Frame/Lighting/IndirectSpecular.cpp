#include "../../PCH.h"
#include "Frame/Lighting/IndirectSpecular.h"

#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Deferred/IndirectSpecularPass.h"
#include "Passes/Core/ShaderPass.h"
#include "RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h"

namespace
{
	bool CanUseDescriptorSceneTlas(const FrameContext& frame) noexcept
	{
		return frame.rayTracingScene.HasBoundTlas() &&
		       frame.rayTracingScene.TlasShaderAccessMode == RayTracingSceneTlasShaderAccessMode::Descriptor;
	}
}

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
	    [&parameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!CanUseDescriptorSceneTlas(frame))
		    {
			    return;
		    }

		    ComputeShaderPass<IndirectSpecularPass::Parameters>::Setup(
		        resourceBuilder,
		        parameters,
		        IndirectSpecularPass::PassName);
	    },
	    [&parameters](PassExecutionContext& context)
	    {
		    if (!CanUseDescriptorSceneTlas(context.Frame))
		    {
			    return;
		    }

		    const IndirectSpecularPass pass(context.RuntimeServices.GetPassRuntime<IndirectSpecularPass>());
		    pass.Execute(context, parameters);
	    });
}
