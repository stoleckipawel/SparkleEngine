#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/NvidiaDlssUpscale.h"

#include "Providers/RendererImageProviderStack.h"

void AddNvidiaDlssUpscalePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    RendererImageProviderStack& imageProviders,
    const UpscalerPassResources& inputs)
{
	AddUpscalerPass(
	    builder,
	    *imageProviders.GetUpscalerProvider(),
	    renderExtent,
	    outputExtent,
	    inputs);
}
