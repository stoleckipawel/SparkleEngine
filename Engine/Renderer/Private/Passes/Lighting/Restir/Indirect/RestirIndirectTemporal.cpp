#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectTemporal.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectTemporalShader.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectShaderParameters.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "ShaderData/SceneShaderParameters.h"

void AddRestirIndirectTemporalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectTemporalCS>();
	parameters->TemporalReservoirSampleTexture = builder.CreateUAV(workingReservoirs.TemporalSample);
	parameters->TemporalReservoirWeightTexture = builder.CreateUAV(workingReservoirs.TemporalWeight);
	parameters->PreviousReservoirSampleTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Sample.Previous);
	parameters->PreviousReservoirWeightTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Weight.Previous);
	parameters->PreviousReservoirSurfaceTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Surface.Previous);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(resources.Transient.GBuffer.WorldNormal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);

	BindSceneShaderParameters(builder, parameters, resources);
	BindRayTracedShadowParameters(builder, parameters);

	const auto invalidateTemporalHistory = [](auto& parameters, bool hasBeenProduced)
	{
		if (!hasBeenProduced)
		{
			ViewTemporalUniformData temporal = *parameters->ViewTemporal.GetValue();
			temporal.HistoryValid = 0u;
			parameters->ViewTemporal = temporal;
		}
	};

	builder.AddResourceProductionSetup(parameters, resources.History.RestirIndirectReservoir.Sample.Previous, invalidateTemporalHistory);
	builder.AddResourceProductionSetup(parameters, resources.History.RestirIndirectReservoir.Weight.Previous, invalidateTemporalHistory);
	builder.AddResourceProductionSetup(parameters, resources.History.RestirIndirectReservoir.Surface.Previous, invalidateTemporalHistory);

	BindRestirIndirectParameters(builder, parameters, resources);

	builder.Dispatch<RestirIndirectTemporalCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
