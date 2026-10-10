#include "../../../PCH.h"
#include "Passes/PostProcessing/Exposure/ExposurePasses.h"

#include "Frame/RenderFrame.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/PostProcessing/Exposure/ExposureAdaptation.h"
#include "Passes/PostProcessing/Exposure/ExposureMeteringPasses.h"

void AddExposurePasses(FrameGraphBuilder& builder, const RenderFrame& frame, const RenderFrameGraphSettings& settings, const RenderFrameGraphResources& resources)
{
	ExposureMomentTexture moments;
	switch (settings.ExposureMeteringMethod)
	{
		case EngineExposureMeteringMethod::Histogram:
			moments = AddExposureHistogramPasses(builder, settings.RenderExtent, resources);
			break;

		case EngineExposureMeteringMethod::DownsamplePyramid:
			moments = AddExposureDownsamplePasses(builder, settings.RenderExtent, resources);
			break;
		default:
			throw Diagnostics::Error("Exposure graph construction received an invalid metering method.");
	}

	AddExposureAdaptationPass(builder, frame, moments, resources);
}
