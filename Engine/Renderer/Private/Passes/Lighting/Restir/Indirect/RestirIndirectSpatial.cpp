#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectSpatial.h"

#include "Frame/RenderFrame.h"
#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectSpatialShader.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectShaderParameters.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "ShaderData/SceneShaderParameters.h"

void AddRestirIndirectSpatialPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectSpatialCS>();
	parameters->TemporalReservoirSampleTexture = builder.CreateSRV(workingReservoirs.TemporalSample);
	parameters->TemporalReservoirWeightTexture = builder.CreateSRV(workingReservoirs.TemporalWeight);
	parameters->CurrentReservoirSampleTexture = builder.CreateUAV(resources.History.RestirIndirectReservoir.Sample.Current);
	parameters->CurrentReservoirWeightTexture = builder.CreateUAV(resources.History.RestirIndirectReservoir.Weight.Current);
	parameters->CurrentReservoirSurfaceTexture = builder.CreateUAV(resources.History.RestirIndirectReservoir.Surface.Current);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(resources.Transient.GBuffer.WorldNormal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);

	BindSceneShaderParameters(builder, frame, parameters, resources);
	BindRayTracedShadowParameters(frame.PreparedScene, parameters);

	BindRestirIndirectParameters(parameters, resources);

	builder.Dispatch<RestirIndirectSpatialCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
