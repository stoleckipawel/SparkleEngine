#include "../../PCH.h"
#include "Frame/PostProcessing/Exposure.h"

#include "Frame/PostProcessing/ExposureMomentChain.h"
#include "Frame/Presentation/ToneMappingCVars.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/ExposurePass.h"

void AddExposurePass(
	FrameGraphBuilder& builder,
	RenderViewportExtent sceneExtent,
	FrameGraphTextureHandle finalSceneColor,
	const FrameGraphTextureHistory& history,
	FrameGraphTextureHandle exposure)
{
	ExposureMomentChain::Texture moments;
	switch (CVarExposureMeteringMethod.Get())
	{
		case EngineExposureMeteringMethod::ParallelReduction:
			moments = ExposureMomentChain::AddReduction(builder, sceneExtent, finalSceneColor);
			break;
		case EngineExposureMeteringMethod::DownsamplePyramid:
			moments = ExposureMomentChain::AddDownsample(builder, sceneExtent, finalSceneColor);
			break;
		default:
		{
			static const auto logger = Logging::GetOrCreateLogger("Renderer.Exposure");
			Diagnostics::Fatal(logger, __FILE__, __LINE__, "Exposure settings contain an unknown metering method.");
		}
	}

	auto& parameters = builder.AllocParameters<ExposurePass::Parameters>();
	parameters->LuminanceMoments = builder.CreateSRV(moments.TextureHandle);
	parameters->PreviousExposureTexture = builder.CreateSRV(history.Previous);
	parameters->ExposureHistoryTexture = builder.CreateUAV(history.Current);
	parameters->ExposureTexture = builder.CreateUAV(exposure);
	builder.DispatchAsync<ExposurePass>(parameters);
}
