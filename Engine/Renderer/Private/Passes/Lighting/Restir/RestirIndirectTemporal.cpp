#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirIndirectTemporal.h"

#include "ShaderData/RayTracingLightingParameters.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectTemporalPass.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"

void AddRestirIndirectTemporalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectTemporalPass::Parameters>();
	parameters->TemporalReservoirSampleTexture = builder.CreateUAV(workingReservoirs.TemporalSample);
	parameters->TemporalReservoirWeightTexture = builder.CreateUAV(workingReservoirs.TemporalWeight);
	parameters->PreviousReservoirSampleTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Sample.Previous);
	parameters->PreviousReservoirWeightTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Weight.Previous);
	parameters->PreviousReservoirSurfaceTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Surface.Previous);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);
	parameters->SceneTlas = builder.Read(resources.SceneTlas);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(resources.Transient.GBuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	parameters->SkyTexture = builder.CreateSRV(resources.ImportedScene.Sky);
	parameters->DirectionalLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.PointLights);
	parameters->SpotLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.SpotLights);
	parameters->RectLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.RectLights);
	parameters->RayTracingHitVertices = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Vertices);
	parameters->MorphTargetDeltas = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.MorphTargetDeltas);
	parameters->SkinInfluences = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.SkinInfluences);
	parameters->RayTracingHitIndices = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Indices);
	parameters->RayTracingHitInstances = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Instances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Materials);
	parameters->MeshInstances = builder.CreateSRV(resources.ImportedScene.Scene.Geometry.MeshInstances);
	parameters->JointMatrices = builder.CreateSRV(resources.ImportedScene.Scene.Geometry.JointMatrices);
	parameters->MorphWeights = builder.CreateSRV(resources.ImportedScene.Scene.Geometry.MorphWeights);
	RegisterRayTracingLightingParameterSetups(builder, parameters);
	const auto invalidateTemporalHistory = [](auto& fields, bool hasBeenProduced)
	{
		if (!hasBeenProduced)
		{
			ViewTemporalUniformData temporal = *fields.ViewTemporal.GetValue();
			temporal.HistoryValid = 0u;
			fields.ViewTemporal = temporal;
		}
	};
	builder.AddResourceProductionSetup(
	    parameters,
	    resources.History.RestirIndirectReservoir.Sample.Previous,
	    invalidateTemporalHistory);
	builder.AddResourceProductionSetup(
	    parameters,
	    resources.History.RestirIndirectReservoir.Weight.Previous,
	    invalidateTemporalHistory);
	builder.AddResourceProductionSetup(
	    parameters,
	    resources.History.RestirIndirectReservoir.Surface.Previous,
	    invalidateTemporalHistory);
	builder.AddPassParameterSetup(
	    parameters,
	    [](auto& fields)
	    {
		    const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
		    fields.RestirIndirectConstants = RestirIndirectLightingUniformData{
		        .BounceCount = settings.BounceCount,
		        .NormalBias = settings.NormalBias,
		        .MaxDistance = settings.MaxDistance};
	    });
	builder.Dispatch<RestirIndirectTemporalPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
