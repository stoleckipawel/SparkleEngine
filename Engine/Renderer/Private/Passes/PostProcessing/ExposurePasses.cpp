#include "../../PCH.h"
#include "Passes/PostProcessing/ExposurePasses.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/PostProcessing/ExposureAdaptation.h"
#include "Passes/PostProcessing/ExposureMeteringPasses.h"

void AddExposurePasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, const RenderFrameGraphResources& resources)
{
	ExposureMomentTexture moments;
	switch (settings.ExposureMeteringMethod)
	{
		case EngineExposureMeteringMethod::ParallelReduction:
			moments = AddExposureReductionPasses(builder, settings.RenderExtent, resources);
			break;
		case EngineExposureMeteringMethod::DownsamplePyramid:
			moments = AddExposureDownsamplePasses(builder, settings.RenderExtent, resources);
			break;
		default:
		{
			static const auto logger = Logging::GetOrCreateLogger("Renderer.Exposure");
			Diagnostics::Fatal(logger, __FILE__, __LINE__, "Exposure settings contain an unknown metering method.");
		}
	}

	AddExposureAdaptationPass(builder, moments, resources);
}
