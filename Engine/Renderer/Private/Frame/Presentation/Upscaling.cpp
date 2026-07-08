#include "../../PCH.h"
#include "Frame/Presentation/Upscaling.h"

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "Passes/Presentation/LinearUpscalePass.h"
#include "Upscaling/UpscalerInputContractBuilder.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"
#include "Upscaling/UpscalerSubsystem.h"

namespace
{
	bool HasSameExtent(RenderViewportExtent a, RenderViewportExtent b) noexcept
	{
		return a.Width == b.Width && a.Height == b.Height;
	}

	FrameGraphTextureHandle ResolveUpscalingInputColor(const FrameAssemblyResourceLayout& resources) noexcept
	{
		return resources.ReconstructedSceneColorProduced
		           ? resources.Transient.Scene.ReconstructedSceneColor
		           : resources.Transient.Scene.SceneColor;
	}

	FrameUpscalerProviderResources BuildUpscalerProviderInputs(
	    FrameGraphTextureHandle scalingInputColor,
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphTextureHandle exposure) noexcept
	{
		return FrameUpscalerProviderResources{
		    .ScalingInputColor = scalingInputColor,
		    .ScalingOutputColor = sceneTargets.FinalSceneColor,
		    .Depth = sceneTargets.MainDepth,
		    .MotionVectors = gbuffer.MotionVector,
		    .Exposure = exposure};
	}

	void AddRendererLinearUpscalePass(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle scalingInputColor,
	    FrameGraphTextureHandle scalingOutputColor,
	    RenderViewportExtent outputExtent)
	{
		auto& parameters = builder.AllocPassParameters<LinearUpscalePass>();
		LinearUpscalePass::DeclareResources(builder, scalingInputColor, scalingOutputColor, parameters);
		builder.AddSizedComputeShaderPass<LinearUpscalePass>(parameters, outputExtent.Width, outputExtent.Height);
	}

	void AddRendererLinearUpscaleFallbackIfNeeded(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle scalingInputColor,
	    FrameGraphTextureHandle scalingOutputColor,
	    RenderViewportExtent renderExtent,
	    RenderViewportExtent outputExtent)
	{
		if (!HasSameExtent(renderExtent, outputExtent))
		{
			AddRendererLinearUpscalePass(builder, scalingInputColor, scalingOutputColor, outputExtent);
		}
	}

	void AddUpscalerEvaluationPass(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent renderExtent,
	    RenderViewportExtent outputExtent,
	    const FrameUpscalerProviderResources& providerInputs)
	{
		builder.AddPass(
		    "UpscalerEvaluation",
		    EFrameGraphPassFlags::ExternalProvider,
		    [providerInputs](PassResourceBuilder& resourceBuilder)
		    {
			    resourceBuilder.Read(providerInputs.ScalingInputColor, ResourceUsage::ShaderRead, "ScalingInputColor");
			    resourceBuilder.Read(providerInputs.Depth, ResourceUsage::DepthRead, "Depth");
			    resourceBuilder.Read(providerInputs.MotionVectors, ResourceUsage::ShaderRead, "MotionVectors");
			    resourceBuilder.Write(providerInputs.ScalingOutputColor, ResourceUsage::UnorderedAccess, "ScalingOutputColor");
		    },
		    [providerInputs, renderExtent, outputExtent](PassExecutionContext& context)
		    {
			    UpscalerEvaluationResult result{
			        .ProducedOutput = false,
			        .FailureDomain = EUpscalerProviderFailureDomain::Backend,
			        .Reason = "No upscaler runtime service was provided."};

			    if (context.RuntimeServices.ImageProviders != nullptr &&
			        context.RuntimeServices.ImageProviders->Upscaling != nullptr)
			    {
				    result = context.RuntimeServices.ImageProviders->Upscaling->Evaluate(
				        UpscalerEvaluationDesc{
				            .ScalingInputColor = ToRenderProductHandle(providerInputs.ScalingInputColor),
				            .Depth = ToRenderProductHandle(providerInputs.Depth),
				            .MotionVectors = ToRenderProductHandle(providerInputs.MotionVectors),
				            .ScalingOutputColor = ToRenderProductHandle(providerInputs.ScalingOutputColor),
				            .BackendApi = context.Commands.GetRenderCommandList().GetBackendApi(),
				            .NativeCommandList = context.Commands.GetRenderCommandList().GetNativeHandle(
				                RhiNativeInteropRequest{
				                    .Consumer = ERhiNativeInteropConsumer::UpscalerProvider,
				                    .Reason = "Evaluate upscaler pass"}),
				            .NativeScalingInputColor = context.Resources.ResolveResource(providerInputs.ScalingInputColor),
				            .NativeDepth = context.Resources.ResolveResource(providerInputs.Depth),
				            .NativeMotionVectors = context.Resources.ResolveResource(providerInputs.MotionVectors),
				            .NativeScalingOutputColor = context.Resources.ResolveResource(providerInputs.ScalingOutputColor),
				            .NativeScalingInputColorView =
				                context.Resources.ResolveNativeTextureView(providerInputs.ScalingInputColor, ResourceState::ShaderResource),
				            .NativeDepthView =
				                context.Resources.ResolveNativeTextureView(providerInputs.Depth, ResourceState::DepthRead),
				            .NativeMotionVectorsView =
				                context.Resources.ResolveNativeTextureView(providerInputs.MotionVectors, ResourceState::ShaderResource),
				            .NativeScalingOutputColorView =
				                context.Resources.ResolveNativeTextureView(providerInputs.ScalingOutputColor, ResourceState::UnorderedAccess),
				            .RenderExtent = renderExtent,
				            .OutputExtent = outputExtent});
			    }

			    if (!result.ProducedOutput && HasSameExtent(renderExtent, outputExtent))
			    {
				    context.Commands.TransitionResource(
				        context.Resources.ResolveResource(providerInputs.ScalingInputColor),
				        ResourceState::ShaderResource,
				        ResourceState::CopySource);
				    context.Commands.TransitionResource(
				        context.Resources.ResolveResource(providerInputs.ScalingOutputColor),
				        ResourceState::UnorderedAccess,
				        ResourceState::CopyDest);
				    context.Resources.CopyTexture(context.Commands, providerInputs.ScalingOutputColor, providerInputs.ScalingInputColor);
				    context.Commands.TransitionResource(
				        context.Resources.ResolveResource(providerInputs.ScalingInputColor),
				        ResourceState::CopySource,
				        ResourceState::ShaderResource);
				    context.Commands.TransitionResource(
				        context.Resources.ResolveResource(providerInputs.ScalingOutputColor),
				        ResourceState::CopyDest,
				        ResourceState::UnorderedAccess);
			    }
		    });
	}
}

UpscalerInputContract BuildFrameUpscalerInputContract(
    const FrameUpscalerProviderResources& providerInputs,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
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
	        .RenderExtent = renderExtent,
	        .OutputExtent = outputExtent,
	        .FrameIndex = frameIndex,
	        .Camera = camera,
	        .TemporalData = temporalData,
	        .TemporalState = temporalState});
}

void AddUpscalingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    FrameAssemblyResourceLayout& resources)
{
	const FrameGraphTextureHandle scalingInputColor = ResolveUpscalingInputColor(resources);
	resources.UpscalerProviderInputs = BuildUpscalerProviderInputs(
	    scalingInputColor,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.Transient.Exposure);

	const UpscalerSettings settings = BuildUpscalerSettingsFromCVars();
	if (settings.RequestedProvider == EUpscalerProviderKind::NvidiaDlss)
	{
		AddRendererLinearUpscaleFallbackIfNeeded(
		    builder,
		    scalingInputColor,
		    resources.Transient.Scene.FinalSceneColor,
		    renderExtent,
		    outputExtent);
		AddUpscalerEvaluationPass(builder, renderExtent, outputExtent, resources.UpscalerProviderInputs);
	}
	else
	{
		AddRendererLinearUpscalePass(
		    builder,
		    scalingInputColor,
		    resources.Transient.Scene.FinalSceneColor,
		    outputExtent);
	}

	resources.FinalSceneColorProduced = true;
}
	void AddRendererLinearUpscaleFallbackIfNeeded(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle scalingInputColor,
	    FrameGraphTextureHandle scalingOutputColor,
	    RenderViewportExtent renderExtent,
	    RenderViewportExtent outputExtent)
	{
		if (!HasSameExtent(renderExtent, outputExtent))
		{
			AddRendererLinearUpscalePass(builder, scalingInputColor, scalingOutputColor, outputExtent);
		}
	}
