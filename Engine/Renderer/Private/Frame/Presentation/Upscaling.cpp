#include "../../PCH.h"
#include "Frame/Presentation/Upscaling.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "Commands/RenderCommandContext.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSubsystem.h"

void AddExternalProviderEvaluationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer)
{
	builder.AddPass(
	    "EvaluateExternalUpscalerProvider",
	    EFrameGraphPassFlags::ExternalProvider,
	    [sceneTargets, gbuffer](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Read(sceneTargets.SceneColor, ResourceUsage::CopySource, "HudlessSceneColor");
		    resourceBuilder.Read(sceneTargets.MainDepth, ResourceUsage::DepthRead, "Depth");
		    resourceBuilder.Read(gbuffer.MotionVector, ResourceUsage::ShaderRead, "MotionVectors");
		    resourceBuilder.Write(sceneTargets.FinalSceneColor, ResourceUsage::UnorderedAccess, "FinalSceneColor");
	    },
	    [sceneTargets, gbuffer, sceneExtent](PassExecutionContext& context)
	    {
		    UpscalerEvaluationResult result{
		        .ProducedOutput = false,
		        .UsedFallback = true,
		        .FailureDomain = EUpscalerProviderFailureDomain::Backend,
		        .Reason = "No upscaler runtime service was provided."};

		    if (context.RuntimeServices.Upscaling != nullptr && context.RuntimeServices.Upscaling->Subsystem != nullptr)
		    {
			    result = context.RuntimeServices.Upscaling->Subsystem->Evaluate(
			        UpscalerEvaluationDesc{
			            .InputColor = ToRenderProductHandle(sceneTargets.SceneColor),
			            .Depth = ToRenderProductHandle(sceneTargets.MainDepth),
			            .MotionVectors = ToRenderProductHandle(gbuffer.MotionVector),
			            .OutputColor = ToRenderProductHandle(sceneTargets.FinalSceneColor),
			            .BackendApi = context.Commands.GetRenderCommandList().GetBackendApi(),
			            .NativeCommandList = context.Commands.GetRenderCommandList().GetNativeHandle(
			                RhiNativeInteropRequest{
			                    .Consumer = ERhiNativeInteropConsumer::UpscalerProvider,
			                    .Reason = "Evaluate external upscaler provider pass"}),
			            .NativeInputColor = context.Resources.ResolveResource(sceneTargets.SceneColor),
			            .NativeDepth = context.Resources.ResolveResource(sceneTargets.MainDepth),
			            .NativeMotionVectors = context.Resources.ResolveResource(gbuffer.MotionVector),
			            .NativeOutputColor = context.Resources.ResolveResource(sceneTargets.FinalSceneColor),
			            .NativeInputColorView = context.Resources.ResolveNativeTextureView(sceneTargets.SceneColor, ResourceState::CopySource),
			            .NativeDepthView = context.Resources.ResolveNativeTextureView(sceneTargets.MainDepth, ResourceState::DepthRead),
			            .NativeMotionVectorsView = context.Resources.ResolveNativeTextureView(gbuffer.MotionVector, ResourceState::ShaderResource),
			            .NativeOutputColorView = context.Resources.ResolveNativeTextureView(sceneTargets.FinalSceneColor, ResourceState::UnorderedAccess),
			            .RenderExtent = sceneExtent,
			            .OutputExtent = sceneExtent});
		    }

		    if (!result.ProducedOutput || result.UsedFallback)
		    {
			    context.Commands.TransitionResource(
			        context.Resources.ResolveResource(sceneTargets.FinalSceneColor),
			        ResourceState::UnorderedAccess,
			        ResourceState::CopyDest);
			    context.Resources.CopyTexture(context.Commands, sceneTargets.FinalSceneColor, sceneTargets.SceneColor);
			    context.Commands.TransitionResource(
			        context.Resources.ResolveResource(sceneTargets.FinalSceneColor),
			        ResourceState::CopyDest,
			        ResourceState::UnorderedAccess);
		    }
	    });
}
