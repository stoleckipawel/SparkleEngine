#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirIndirectSpatial.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectSpatialShader.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "ShaderData/SceneShaderParameters.h"

void AddRestirIndirectSpatialPass(
    FrameGraphBuilder& builder,
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
	parameters->GBufferNormal = builder.CreateSRV(resources.Transient.GBuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	BindSceneShaderParameters(builder, parameters, resources);
	BindRayTracedShadowParameters(builder, parameters);
	builder.AddPassParameterSetup(
	    parameters,
	    [](auto& fields)
	    {
		    const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
		    fields.RestirIndirectConstants = RestirIndirectLightingUniformData{.BounceCount = settings.BounceCount};
	    });
	builder.Dispatch<RestirIndirectSpatialCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
