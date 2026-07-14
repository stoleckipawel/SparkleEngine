#include "../../PCH.h"
#include "Frame/Lighting/RestirIndirectTemporal.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectTemporalPass.h"

void AddRestirIndirectTemporalPass(
    FrameGraphBuilder& builder,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const FrameAssemblyResourceLayout& resources)
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
	parameters->SkyTexture = builder.CreateSRV(resources.External.Sky);
	parameters->DirectionalLights = builder.CreateSRV(resources.External.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(resources.External.PointLights);
	parameters->SpotLights = builder.CreateSRV(resources.External.SpotLights);
	parameters->RectLights = builder.CreateSRV(resources.External.RectLights);
	parameters->RayTracingHitVertices = builder.CreateSRV(resources.External.RayTracingHitVertices);
	parameters->SkinInfluences = builder.CreateSRV(resources.External.RayTracingHitSkinInfluences);
	parameters->RayTracingHitIndices = builder.CreateSRV(resources.External.RayTracingHitIndices);
	parameters->RayTracingHitInstances = builder.CreateSRV(resources.External.RayTracingHitInstances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(resources.External.RayTracingHitMaterials);
	parameters->MeshInstances = builder.CreateSRV(resources.External.MeshInstances);
	parameters->JointMatrices = builder.CreateSRV(resources.External.JointMatrices);
	LightingRayTracingPasses::DispatchSceneTlas<RestirIndirectTemporalPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
