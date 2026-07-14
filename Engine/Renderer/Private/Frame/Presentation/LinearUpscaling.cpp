#include "../../PCH.h"
#include "Frame/Presentation/LinearUpscaling.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/LinearUpscalePass.h"

void AddLinearUpscalePass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle inputColor,
    FrameGraphTextureHandle outputColor,
    RenderViewportExtent outputExtent)
{
	auto& parameters = builder.AllocParameters<LinearUpscalePass::Parameters>();
	parameters->ScalingInputColor = builder.CreateSRV(inputColor);
	parameters->ScalingOutputColor = builder.CreateUAV(outputColor);
	builder.Dispatch<LinearUpscalePass>(parameters, outputExtent.Width, outputExtent.Height);
}
