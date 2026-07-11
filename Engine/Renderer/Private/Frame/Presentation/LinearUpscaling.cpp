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
	auto& parameters = builder.AllocPassParameters<LinearUpscalePass>();
	LinearUpscalePass::DeclareResources(builder, inputColor, outputColor, parameters);
	builder.AddSizedComputeShaderPass<LinearUpscalePass>(parameters, outputExtent.Width, outputExtent.Height);
}
