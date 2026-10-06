#include "../../../PCH.h"
#include "Passes/PostProcessing/Exposure/ExposureAdaptation.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
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
	    [](auto& parameters, const RenderView& view)
	    {
		    parameters->ExposureMode = static_cast<std::uint32_t>(view.displaySettings.ExposureMode);
		    parameters->ExposureHistoryValid = 0u;
		    parameters->ManualExposure = view.displaySettings.ManualExposure;
		    parameters->ExposureCompensation = view.displaySettings.ExposureCompensation;
		    parameters->ExposureTargetLuminance = view.displaySettings.ExposureTargetLuminance;
		    parameters->ExposureMin = view.displaySettings.ExposureMin;
		    parameters->ExposureMax = view.displaySettings.ExposureMax;
		    parameters->ExposureAdaptationSpeedUp = view.displaySettings.ExposureAdaptationSpeedUp;
		    parameters->ExposureAdaptationSpeedDown = view.displaySettings.ExposureAdaptationSpeedDown;
	    });

	builder.AddParameterSetup<FrameUniformData>(
	    parameters,
	    [](auto& parameters, const FrameUniformData& frame) { parameters->Frame = frame; });

	builder.AddResourceProductionSetup(
	    parameters,
	    resources.History.Exposure.Previous,
	    [](auto& parameters, bool hasBeenProduced) { parameters->ExposureHistoryValid = hasBeenProduced ? 1u : 0u; });

	builder.DispatchAsync<ExposureCS>(parameters, ComputeDispatchDesc{1u, 1u, 1u});
}
