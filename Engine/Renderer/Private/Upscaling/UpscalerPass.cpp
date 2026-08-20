#include "../PCH.h"
#include "Upscaling/UpscalerPass.h"

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Upscaling/UpscalerProvider.h"

static const auto g_upscalerPassLogger = Logging::GetOrCreateLogger("Renderer.UpscalerPass");

void AddUpscalerPass(
    FrameGraphBuilder& builder,
    IUpscalerProvider& provider,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    const UpscalerPassResources& inputs)
{
	if (!inputs.InputColor.IsValid() || !inputs.OutputColor.IsValid() || !inputs.Depth.IsValid() || !inputs.MotionVectors.IsValid()
	    || !inputs.Exposure.IsValid())
	{
		Diagnostics::Fatal(g_upscalerPassLogger, __FILE__, __LINE__, "Upscaler pass received an incomplete resource set.");
	}

	builder.AddPass(
	    "Upscaler",
	    EFrameGraphPassKind::ExternalProvider,
	    [inputs](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Read(inputs.InputColor, ResourceUsage::ShaderRead, "InputColor");
		    resourceBuilder.Read(inputs.Depth, ResourceUsage::ShaderRead, "Depth");
		    resourceBuilder.Read(inputs.MotionVectors, ResourceUsage::ShaderRead, "MotionVectors");
		    resourceBuilder.Read(inputs.Exposure, ResourceUsage::ShaderRead, "Exposure");
		    resourceBuilder.Use(inputs.OutputColor, ResourceUsage::UnorderedAccess, "OutputColor");
	    },
	    [&provider, inputs, renderExtent, outputExtent](PassCommandContext& context)
	    {
		    RenderCommandList& commandList = context.Commands.GetRenderCommandList();
		    const RhiNativeInteropRequest interopRequest{
		        .Consumer = ERhiNativeInteropConsumer::ExternalProvider,
		        .Reason = "Evaluate upscaler pass"};
		    if (!provider.Evaluate(
		            UpscalerEvaluationDesc{
		                .BackendApi = commandList.GetBackendApi(),
		                .NativeCommandList = commandList.GetNativeHandle(interopRequest),
		                .NativeScalingInputColorView =
		                    context.Resources.ResolveNativeTextureView(inputs.InputColor, ResourceState::ShaderResource, interopRequest),
		                .NativeDepthView =
		                    context.Resources.ResolveNativeTextureView(inputs.Depth, ResourceState::ShaderResource, interopRequest),
		                .NativeMotionVectorsView =
		                    context.Resources.ResolveNativeTextureView(inputs.MotionVectors, ResourceState::ShaderResource, interopRequest),
		                .NativeExposureView =
		                    context.Resources.ResolveNativeTextureView(inputs.Exposure, ResourceState::ShaderResource, interopRequest),
		                .NativeScalingOutputColorView =
		                    context.Resources.ResolveNativeTextureView(inputs.OutputColor, ResourceState::UnorderedAccess, interopRequest),
		                .RenderExtent = renderExtent,
		                .OutputExtent = outputExtent}))
		    {
			    Diagnostics::Fatal(
			        g_upscalerPassLogger,
			        __FILE__,
			        __LINE__,
			        "The configured upscaler failed to evaluate the active frame.");
		    }
	    });
}
