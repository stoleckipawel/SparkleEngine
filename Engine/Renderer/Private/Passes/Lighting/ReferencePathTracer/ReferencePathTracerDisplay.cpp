#include "PCH.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerDisplay.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerResources.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerShader.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerWork.h"

void AddReferencePathTracerDisplayPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerWork& work)
{
	auto& parameters = builder.AllocParameters<ReferencePathTracerDisplayCS>();
	parameters->WorkingMean = builder.CreateSRV(graphResources.WorkingMean);
	parameters->WorkingM2 = builder.CreateSRV(graphResources.WorkingM2);
	parameters->CommittedMean = builder.CreateUAV(graphResources.CommittedMean);
	parameters->CommittedM2 = builder.CreateUAV(graphResources.CommittedM2);
	parameters->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);

	parameters->SessionSeed = work.SessionSeed;
	parameters->ReplicateId = work.ReplicateId;
	parameters->SampleOrdinal = work.SampleOrdinal;
	parameters->FinitePathDiagnosticSurfaceVertices = work.FinitePathDiagnosticSurfaceVertices;
	parameters->FirstRow = work.FirstRow;
	parameters->RowCount = work.RowCount;
	parameters->PriorSampleCount = work.PriorSampleCount;
	parameters->WorkFlags = work.WorkFlags;

	builder.Dispatch<ReferencePathTracerDisplayCS>(
	    "ReferencePathTracer.CommittedDisplay",
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(extent.Width, 8u), MathUtils::DivideRoundUp(extent.Height, 8u), 1u});
}
