#include "../../PCH.h"
#include "Passes/PostProcessing/Exposure.h"

#include "Passes/PostProcessing/ExposureMomentChain.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/ExposureShader.h"

void AddExposurePass(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, const RenderFrameGraphResources& resources)
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

	auto& parameters = builder.AllocParameters<ExposureCS>();
	parameters->LuminanceMoments = builder.CreateSRV(moments.TextureHandle);
	parameters->PreviousExposureTexture = builder.CreateSRV(resources.History.Exposure.Previous);
	parameters->ExposureHistoryTexture = builder.CreateUAV(resources.History.Exposure.Current);
	parameters->ExposureTexture = builder.CreateUAV(resources.Transient.Exposure);
	builder.AddParameterSetup<ExposureUniformData>(
	    parameters,
	    [](auto& fields, const ExposureUniformData& exposure) { fields.ExposureConstants = exposure; });
	builder.AddResourceProductionSetup(
	    parameters,
	    resources.History.Exposure.Previous,
	    [](auto& fields, bool hasBeenProduced)
	    {
		    ExposureUniformData exposure = *fields.ExposureConstants.GetValue();
		    exposure.ExposureHistoryValid = hasBeenProduced ? 1u : 0u;
		    fields.ExposureConstants = exposure;
	    });
	builder.DispatchAsync<ExposureCS>(parameters, ComputeDispatchDesc{1u, 1u, 1u});
}
