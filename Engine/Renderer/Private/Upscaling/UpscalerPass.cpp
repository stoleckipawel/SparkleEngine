#include "../PCH.h"
#include "Upscaling/UpscalerPass.h"

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Upscaling/UpscalerProvider.h"

namespace
{
	bool HasRequiredInputs(const UpscalerPassResources& inputs) noexcept
	{
		return inputs.InputColor.IsValid() && inputs.OutputColor.IsValid() && inputs.Depth.IsValid() &&
		       inputs.MotionVectors.IsValid() && inputs.Exposure.IsValid();
	}
}

void AddUpscalerPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    const UpscalerPassResources& inputs)
{
	if (!HasRequiredInputs(inputs))
	{
		return;
	}

	builder.AddPass(
	    "Upscaler",
	    EFrameGraphPassFlags::ExternalProvider,
	    [inputs](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Read(inputs.InputColor, ResourceUsage::ShaderRead, "InputColor");
		    resourceBuilder.Read(inputs.Depth, ResourceUsage::ShaderRead, "Depth");
		    resourceBuilder.Read(inputs.MotionVectors, ResourceUsage::ShaderRead, "MotionVectors");
		    resourceBuilder.Read(inputs.Exposure, ResourceUsage::ShaderRead, "Exposure");
		    resourceBuilder.Write(inputs.OutputColor, ResourceUsage::UnorderedAccess, "OutputColor");
	    },
	    [inputs, renderExtent, outputExtent](PassExecutionContext& context)
	    {
		    if (context.RuntimeServices.ImageProviders == nullptr ||
		        context.RuntimeServices.ImageProviders->Upscaling == nullptr)
		    {
			    return;
		    }

		    RenderCommandList& commandList = context.Commands.GetRenderCommandList();
		    (void) context.RuntimeServices.ImageProviders->Upscaling->Evaluate(
		        UpscalerEvaluationDesc{
		            .BackendApi = commandList.GetBackendApi(),
		            .NativeCommandList = commandList.GetNativeHandle(
		                RhiNativeInteropRequest{
		                    .Consumer = ERhiNativeInteropConsumer::UpscalerProvider,
		                    .Reason = "Evaluate upscaler pass"}),
		            .NativeScalingInputColorView =
		                context.Resources.ResolveNativeTextureView(inputs.InputColor, ResourceState::ShaderResource),
		            .NativeDepthView =
		                context.Resources.ResolveNativeTextureView(inputs.Depth, ResourceState::ShaderResource),
		            .NativeMotionVectorsView =
		                context.Resources.ResolveNativeTextureView(inputs.MotionVectors, ResourceState::ShaderResource),
		            .NativeExposureView =
		                context.Resources.ResolveNativeTextureView(inputs.Exposure, ResourceState::ShaderResource),
		            .NativeScalingOutputColorView =
		                context.Resources.ResolveNativeTextureView(inputs.OutputColor, ResourceState::UnorderedAccess),
		            .RenderExtent = renderExtent,
		            .OutputExtent = outputExtent});
	    });
}
