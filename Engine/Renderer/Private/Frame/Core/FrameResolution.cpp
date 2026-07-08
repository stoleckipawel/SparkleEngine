#include "../../PCH.h"
#include "Frame/Core/FrameResolution.h"

#include "Upscaling/UpscalerSettings.h"

FrameResolutionExtents ResolveFrameResolutionExtents(
    RenderViewportExtent outputExtent,
    FrameRenderPath renderPath) noexcept
{
	if (renderPath == FrameRenderPath::PathTracedReference)
	{
		return FrameResolutionExtents{
		    .Render = outputExtent,
		    .Output = outputExtent};
	}

	const UpscalerSettings settings = BuildUpscalerSettingsFromCVars();
	return FrameResolutionExtents{
	    .Render = ResolveUpscalerRenderExtent(outputExtent, settings.QualityMode),
	    .Output = outputExtent};
}
