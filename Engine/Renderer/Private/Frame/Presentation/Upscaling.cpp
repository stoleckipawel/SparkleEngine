#include "../../PCH.h"
#include "Frame/Presentation/Upscaling.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "Commands/RenderCommandContext.h"
#include "Upscaling/UpscalerInputContractBuilder.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSubsystem.h"

UpscalerInputContract BuildFrameUpscalerInputContract(
    const FrameAssemblyUpscalerProviderResources& providerInputs,
    RenderViewportExtent sceneExtent,
    std::uint64_t frameIndex,
    const PerViewCameraConstantBufferData& camera,
    const PerTemporalConstantBufferData& temporalData,
    RenderTemporalFrameState temporalState)
{
	return BuildUpscalerInputContract(
	    UpscalerInputContractBuildDesc{
	        .ScalingInputColor = ToRenderProductHandle(providerInputs.ScalingInputColor),
	        .Depth = ToRenderProductHandle(providerInputs.Depth),
	        .MotionVectors = ToRenderProductHandle(providerInputs.MotionVectors),
	        .Exposure = ToRenderProductHandle(providerInputs.Exposure),
	        .ScalingOutputColor = ToRenderProductHandle(providerInputs.ScalingOutputColor),
	        .RenderExtent = sceneExtent,
	        .OutputExtent = sceneExtent,
	        .FrameIndex = frameIndex,
	        .Camera = camera,
	        .TemporalData = temporalData,
	        .TemporalState = temporalState});
}

void AddUpscalerEvaluationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer)
{
	builder.AddPass(
	    "UpscalerEvaluation",
	    EFrameGraphPassFlags::ExternalProvider,
	    [sceneTargets, gbuffer](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Read(sceneTargets.SceneColor, ResourceUsage::CopySource, "ScalingInputColor");
		    resourceBuilder.Read(sceneTargets.MainDepth, ResourceUsage::DepthRead, "Depth");
		    resourceBuilder.Read(gbuffer.MotionVector, ResourceUsage::ShaderRead, "MotionVectors");
		    resourceBuilder.Write(sceneTargets.FinalSceneColor, ResourceUsage::UnorderedAccess, "ScalingOutputColor");
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
			            .ScalingInputColor = ToRenderProductHandle(sceneTargets.SceneColor),
			            .Depth = ToRenderProductHandle(sceneTargets.MainDepth),
			            .MotionVectors = ToRenderProductHandle(gbuffer.MotionVector),
			            .ScalingOutputColor = ToRenderProductHandle(sceneTargets.FinalSceneColor),
			            .BackendApi = context.Commands.GetRenderCommandList().GetBackendApi(),
			            .NativeCommandList = context.Commands.GetRenderCommandList().GetNativeHandle(
			                RhiNativeInteropRequest{
			                    .Consumer = ERhiNativeInteropConsumer::UpscalerProvider,
			                    .Reason = "Evaluate upscaler pass"}),
			            .NativeScalingInputColor = context.Resources.ResolveResource(sceneTargets.SceneColor),
			            .NativeDepth = context.Resources.ResolveResource(sceneTargets.MainDepth),
			            .NativeMotionVectors = context.Resources.ResolveResource(gbuffer.MotionVector),
			            .NativeScalingOutputColor = context.Resources.ResolveResource(sceneTargets.FinalSceneColor),
			            .NativeScalingInputColorView = context.Resources.ResolveNativeTextureView(sceneTargets.SceneColor, ResourceState::CopySource),
			            .NativeDepthView = context.Resources.ResolveNativeTextureView(sceneTargets.MainDepth, ResourceState::DepthRead),
			            .NativeMotionVectorsView = context.Resources.ResolveNativeTextureView(gbuffer.MotionVector, ResourceState::ShaderResource),
			            .NativeScalingOutputColorView =
			                context.Resources.ResolveNativeTextureView(sceneTargets.FinalSceneColor, ResourceState::UnorderedAccess),
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
