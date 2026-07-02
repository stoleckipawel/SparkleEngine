#include "../../PCH.h"
#include "Frame/Reference/ReferencePathTracing.h"

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Reference/ReferenceRenderTargets.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/ShaderPass.h"
#include "Passes/Reference/ReferencePathTracingPass.h"

namespace
{
	bool CanUseDescriptorSceneTlas(const FrameContext& frame) noexcept
	{
		return RayTracingScenePassBinding::FrameUsesSceneTlasAccessMode(
		    frame,
		    RayTracingSceneTlasShaderAccessMode::Descriptor);
	}
}

void AddReferencePathTracingPass(
    FrameGraphBuilder& builder,
    const ReferenceRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameters = builder.AllocPassParameters<ReferencePathTracingPass>();
	ReferencePathTracingPass::DeclareResources(builder, targets, sceneTlas, parameters);
	builder.AddPass(
	    ReferencePathTracingPass::PassName,
	    EFrameGraphPassFlags::Compute,
	    [&parameters](PassResourceBuilder& resourceBuilder, const FrameContext& frame)
	    {
		    if (!CanUseDescriptorSceneTlas(frame))
		    {
			    return;
		    }

		    ComputeShaderPass<ReferencePathTracingPass::Parameters>::Setup(
		        resourceBuilder,
		        parameters,
		        ReferencePathTracingPass::PassName);
	    },
	    [&parameters](PassExecutionContext& context)
	    {
		    if (!CanUseDescriptorSceneTlas(context.Frame))
		    {
			    return;
		    }

		    const ReferencePathTracingPass pass(context.RuntimeServices.GetPassRuntime<ReferencePathTracingPass>());
		    pass.Execute(context, parameters);
	    });
}

void AddReferenceRenderingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources)
{
	resources.Transient.Reference = CreateReferenceRenderTargets(builder, sceneExtent);
	AddReferenceTargetClearPass(builder, resources.Transient.Reference);
	AddReferencePathTracingPass(builder, resources.Transient.Reference, resources.Persistent.SceneTlas);
	PassUtilities::AddCopyTexturePass(
	    builder,
	    "ReferenceSceneColorToSceneColor",
	    resources.Transient.Scene.SceneColor,
	    resources.Transient.Reference.ReferenceSceneColor);
	PassUtilities::AddCopyTexturePass(
	    builder,
	    "ReferenceSceneColorToFinalSceneColor",
	    resources.Transient.Scene.FinalSceneColor,
	    resources.Transient.Reference.ReferenceSceneColor);
	resources.FinalSceneColorProduced = true;
}
