#include "../../PCH.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"

ImageProviderPipeline ResolveFrameImagePipeline(RenderViewMode viewMode) noexcept
{
	return viewMode == RenderViewMode::ReferencePathTracer ? ImageProviderPipeline::PresentationUpscaling
	                                                       : ImageProviderPipeline::RayReconstruction;
}
