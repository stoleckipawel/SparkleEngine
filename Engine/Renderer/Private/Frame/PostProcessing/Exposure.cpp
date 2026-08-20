#include "../../PCH.h"
#include "Frame/PostProcessing/Exposure.h"

#include "Frame/PostProcessing/ExposureMomentChain.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/ExposurePass.h"

void AddExposurePass(FrameGraphBuilder& builder, const FrameBuildSettings& settings, const FrameAssemblyResourceLayout& resources)
{
	ExposureMomentChain::Texture moments;
	switch (settings.ExposureMeteringMethod)
	{
		case EngineExposureMeteringMethod::ParallelReduction:
			moments = ExposureMomentChain::AddReduction(builder, settings.RenderExtent, resources.Transient.Scene.SceneColor);
			break;
		case EngineExposureMeteringMethod::DownsamplePyramid:
			moments = ExposureMomentChain::AddDownsample(builder, settings.RenderExtent, resources.Transient.Scene.SceneColor);
			break;
		default:
		{
			static const auto logger = Logging::GetOrCreateLogger("Renderer.Exposure");
			Diagnostics::Fatal(logger, __FILE__, __LINE__, "Exposure settings contain an unknown metering method.");
		}
	}

	auto& parameters = builder.AllocParameters<ExposurePass::Parameters>();
	auto* parameterFields = parameters.operator->();
	parameters->LuminanceMoments = builder.CreateSRV(moments.TextureHandle);
	parameters->PreviousExposureTexture = builder.CreateSRV(resources.History.Exposure.Previous);
	parameters->ExposureHistoryTexture = builder.CreateUAV(resources.History.Exposure.Current);
	parameters->ExposureTexture = builder.CreateUAV(resources.Transient.Exposure);
	builder.AddExposureSetup([parameterFields](const ExposureUniformData& exposure) { parameterFields->ExposureConstants = exposure; });
	builder.DispatchAsync<ExposurePass>(parameters);
}
