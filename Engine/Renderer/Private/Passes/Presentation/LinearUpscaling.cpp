#include "../../PCH.h"
#include "Passes/Presentation/LinearUpscaling.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/LinearUpscaleShader.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

void AddLinearUpscalePass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle inputColor,
    FrameGraphTextureHandle outputColor,
    RenderViewportExtent outputExtent)
{
	auto& parameters = builder.AllocParameters<LinearUpscaleCS>();
	parameters->ScalingInputColor = builder.CreateSRV(inputColor);
	parameters->ScalingOutputColor = builder.CreateUAV(outputColor);
	parameters->SamplerLinearClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	builder.Dispatch<LinearUpscaleCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(outputExtent.Width, 8u), MathUtils::DivideRoundUp(outputExtent.Height, 8u), 1u});
}
