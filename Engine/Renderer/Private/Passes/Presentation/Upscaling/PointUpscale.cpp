#include "../../../PCH.h"
#include "Passes/Presentation/Upscaling/PointUpscale.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/Upscaling/PointUpscaleShader.h"

void AddPointUpscalePass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle inputColor,
    FrameGraphTextureHandle outputColor,
    RenderViewportExtent outputExtent)
{
	auto& parameters = builder.AllocParameters<PointUpscaleCS>();
	parameters->ScalingInputColor = builder.CreateSRV(inputColor);
	parameters->ScalingOutputColor = builder.CreateUAV(outputColor);

	builder.Dispatch<PointUpscaleCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(outputExtent.Width, 8u), MathUtils::DivideRoundUp(outputExtent.Height, 8u), 1u});
}
