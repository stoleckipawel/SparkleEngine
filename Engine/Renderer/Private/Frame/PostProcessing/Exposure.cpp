#include "../../PCH.h"
#include "Frame/PostProcessing/Exposure.h"

#include "Frame/PostProcessing/ExposureMomentChain.h"
#include "Frame/Presentation/ToneMappingCVars.h"
#include "Frame/Presentation/ToneMappingSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/ExposurePass.h"

void AddExposurePass(
	FrameGraphBuilder& builder,
	RenderViewportExtent sceneExtent,
	FrameGraphTextureHandle finalSceneColor,
	const FrameGraphTextureHistory& history,
	FrameGraphTextureHandle exposure)
{
	const EngineExposureMeteringMethod meteringMethod = SanitizeExposureMeteringMethod(CVarExposureMeteringMethod.Get());
	const ExposureMomentChain::Texture moments =
	    meteringMethod == EngineExposureMeteringMethod::DownsamplePyramid
	        ? ExposureMomentChain::AddDownsample(builder, sceneExtent, finalSceneColor)
	        : ExposureMomentChain::AddReduction(builder, sceneExtent, finalSceneColor);

	auto& parameters = builder.AllocPassParameters<ExposurePass>();
	ExposurePass::DeclareResources(builder, moments.TextureHandle, history.Previous, history.Current, exposure, parameters);
	builder.AddComputeShaderPass<ExposurePass>(parameters);
}
