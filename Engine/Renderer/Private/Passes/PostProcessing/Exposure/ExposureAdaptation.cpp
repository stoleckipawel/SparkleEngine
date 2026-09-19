#include "../../../PCH.h"
#include "Passes/PostProcessing/Exposure/ExposureAdaptation.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/Exposure/ExposureSettings.h"
#include "Passes/PostProcessing/Exposure/ExposureShader.h"
#include "ShaderData/FrameUniformData.h"
#include "View/RenderView.h"

void AddExposureAdaptationPass(
    FrameGraphBuilder& builder,
    const ExposureMomentTexture& luminanceMoments,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<ExposureCS>();
	parameters->LuminanceMoments = builder.CreateSRV(luminanceMoments.Handle);
	parameters->PreviousExposureTexture = builder.CreateSRV(resources.History.Exposure.Previous);
	parameters->ExposureHistoryTexture = builder.CreateUAV(resources.History.Exposure.Current);
	parameters->ExposureTexture = builder.CreateUAV(resources.Transient.Exposure);

	builder.AddParameterSetup<RenderView>(
	    parameters,
	    [](auto& fields, const RenderView& view) { fields.ExposureConstants = BuildExposureUniformData(view.displaySettings); });

	builder.AddParameterSetup<FrameUniformData>(parameters, [](auto& fields, const FrameUniformData& frame) { fields.Frame = frame; });

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
