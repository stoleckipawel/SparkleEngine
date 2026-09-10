#include "../../../PCH.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerAccumulation.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/ReferencePathTracerAccumulationShader.h"
#include "RayTracing/Effects/ReferencePathTracer/ReferencePathTracerSettings.h"

void AddReferencePathTracerAccumulationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referencePathTracerSample,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<ReferencePathTracerAccumulationCS>();
	parameters->ReferencePathTracerSample = builder.CreateSRV(referencePathTracerSample);
	parameters->SceneColorTexture = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->PreviousReferencePathTracer = builder.CreateSRV(resources.History.ReferencePathTracer.Previous);
	parameters->CurrentReferencePathTracer = builder.CreateUAV(resources.History.ReferencePathTracer.Current);
	parameters->ReferencePathTracerSampleValidity = builder.CreateSRV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);
	builder.AddPassParameterSetup(
	    parameters,
	    [](auto& fields)
	    {
		    fields.ReferencePathTracerAccumulationConstants = ReferencePathTracerAccumulationUniformData{
		        .SamplesPerFrame = BuildReferencePathTracerSettings().SamplesPerPixel,
		        .HistoryValid = 0u};
	    });
	builder.AddResourceProductionSetup(
	    parameters,
	    resources.History.ReferencePathTracer.Previous,
	    [](auto& fields, bool hasBeenProduced)
	    {
		    ReferencePathTracerAccumulationUniformData constants = *fields.ReferencePathTracerAccumulationConstants.GetValue();
		    constants.HistoryValid = hasBeenProduced ? 1u : 0u;
		    fields.ReferencePathTracerAccumulationConstants = constants;
	    });
	builder.Dispatch<ReferencePathTracerAccumulationCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
