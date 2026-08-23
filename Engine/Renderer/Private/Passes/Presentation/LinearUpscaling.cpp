#include "../../PCH.h"
#include "Passes/Presentation/LinearUpscaling.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/LinearUpscalePass.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

void AddLinearUpscalePass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle inputColor,
    FrameGraphTextureHandle outputColor,
    RenderViewportExtent outputExtent)
{
	auto& parameters = builder.AllocParameters<LinearUpscalePass::Parameters>();
	parameters->ScalingInputColor = builder.CreateSRV(inputColor);
	parameters->ScalingOutputColor = builder.CreateUAV(outputColor);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	builder.Dispatch<LinearUpscalePass>(parameters, outputExtent.Width, outputExtent.Height);
}
