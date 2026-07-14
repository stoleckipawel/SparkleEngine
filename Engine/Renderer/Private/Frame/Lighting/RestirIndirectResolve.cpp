#include "../../PCH.h"
#include "Frame/Lighting/RestirIndirectResolve.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectResolvePass.h"

void AddRestirIndirectResolvePass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectResolvePass::Parameters>();
	parameters->CurrentReservoirSampleTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Sample.Current);
	parameters->CurrentReservoirWeightTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Weight.Current);
	parameters->IndirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(resources.Transient.Lighting.IndirectSpecular);
	parameters->RayReconstructionDiffuseAlbedo = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.DiffuseAlbedo);
	parameters->RayReconstructionSpecularAlbedo = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.SpecularAlbedo);
	parameters->RayReconstructionRoughness = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.Roughness);
	parameters->RayReconstructionSpecularHitDistance =
	    builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.SpecularHitDistance);
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
	LightingRayTracingPasses::DispatchSceneTlas<RestirIndirectResolvePass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
