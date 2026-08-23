#include "../../PCH.h"
#include "Passes/Utility/ComputeClear.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

void AddComputeClearPass(
    FrameGraphBuilder& builder,
    std::string_view label,
    FrameGraphTextureHandle outputTexture,
    RenderViewportExtent outputExtent)
{
	auto& parameters = builder.AllocParameters<ComputeClearCS>();
	parameters->Output = builder.CreateUAV(outputTexture);
	builder.Dispatch<ComputeClearCS>(
	    label,
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(outputExtent.Width, 8u), MathUtils::DivideRoundUp(outputExtent.Height, 8u), 1u});
}
