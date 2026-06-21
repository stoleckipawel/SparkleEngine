#include "../PCH.h"
#include "Frame/RTIndirectSpecular.h"

#include "Frame/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/RTIndirectSpecularPass.h"
#include "Passes/ShaderPass.h"
#include "RayTracing/RayTracingSceneTlasShaderAccessMode.h"

namespace RTIndirectSpecularFramePasses
{
	bool UsesDescriptorSceneTlas(const FrameContext& frame) noexcept
	{
		return frame.rayTracingScene.TlasShaderAccessMode == RayTracingSceneTlasShaderAccessMode::Descriptor;
	}
}  // namespace RTIndirectSpecularFramePasses

void AddRTIndirectSpecularPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameters = builder.AllocPassParameters<RTIndirectSpecularPass>();
	RTIndirectSpecularPass::DeclareResources(builder, lighting, gbuffer, sceneTlas, parameters);
	builder.AddPass(
	    RTIndirectSpecularPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&parameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!RTIndirectSpecularFramePasses::UsesDescriptorSceneTlas(frame))
		    {
			    return;
		    }

		    ComputeShaderPass<RTIndirectSpecularPass::Parameters>::Setup(
		        resourceBuilder,
		        parameters,
		        RTIndirectSpecularPass::PassName);
	    },
	    [&parameters](PassExecutionContext& context)
	    {
		    if (!RTIndirectSpecularFramePasses::UsesDescriptorSceneTlas(context.Frame))
		    {
			    return;
		    }

		    const RTIndirectSpecularPass pass(context.RuntimeServices.GetPassRuntime<RTIndirectSpecularPass>());
		    pass.Execute(context, parameters);
	    });
}
