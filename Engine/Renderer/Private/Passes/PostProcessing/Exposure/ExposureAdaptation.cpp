#include "../../../PCH.h"
#include "Passes/PostProcessing/Exposure/ExposureAdaptation.h"

#include "Frame/RenderFrame.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/Exposure/ExposureShader.h"
#include "ShaderData/FrameUniformData.h"
#include "View/RenderView.h"

void AddExposureAdaptationPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    const ExposureMomentTexture& luminanceMoments,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<ExposureCS>();
	parameters->LuminanceMoments = builder.CreateSRV(luminanceMoments.Handle);
	parameters->PreviousExposureTexture = builder.CreateSRV(resources.History.Exposure.Previous);
	parameters->ExposureHistoryTexture = builder.CreateUAV(resources.History.Exposure.Current);
	parameters->ExposureTexture = builder.CreateUAV(resources.Transient.Exposure);

	parameters->ExposureMode = static_cast<std::uint32_t>(frame.View.displaySettings.ExposureMode);
	parameters->ExposureHistoryValid = builder.IsTextureHistoryValid(resources.History.Exposure) ? 1u : 0u;
	parameters->ManualExposure = frame.View.displaySettings.ManualExposure;
	parameters->ExposureCompensation = frame.View.displaySettings.ExposureCompensation;
	parameters->ExposureTargetLuminance = frame.View.displaySettings.ExposureTargetLuminance;
	parameters->ExposureMin = frame.View.displaySettings.ExposureMin;
	parameters->ExposureMax = frame.View.displaySettings.ExposureMax;
	parameters->ExposureAdaptationSpeedUp = frame.View.displaySettings.ExposureAdaptationSpeedUp;
	parameters->ExposureAdaptationSpeedDown = frame.View.displaySettings.ExposureAdaptationSpeedDown;

	parameters->Frame = BuildFrameUniformData(frame.Identity.FrameId, frame.Time);

	builder.DispatchAsync<ExposureCS>(parameters, ComputeDispatchDesc{1u, 1u, 1u});
}
