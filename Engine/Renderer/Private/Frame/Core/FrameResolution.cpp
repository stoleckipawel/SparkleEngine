#include "../../PCH.h"
#include "Frame/Core/FrameResolution.h"

#include "Upscaling/UpscalerSettings.h"

FrameResolutionExtents ResolveFrameResolutionExtents(RenderViewportExtent outputExtent) noexcept
{
	return FrameResolutionExtents{
	    .Render = ResolveUpscalerRenderExtent(outputExtent, CVarUpscalerQualityMode.Get()),
	    .Output = outputExtent};
}
