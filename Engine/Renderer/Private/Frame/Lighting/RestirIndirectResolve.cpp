#include "../../PCH.h"
#include "Frame/Lighting/RestirIndirectResolve.h"

#include "Frame/Lighting/RayTracingLightingParameters.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectResolvePass.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"

void AddRestirIndirectResolvePass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectResolvePass::Parameters>();
	auto* parameterFields = parameters.operator->();
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
	parameters->DirectionalLights = builder.CreateSRV(resources.External.Scene.Lighting.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(resources.External.Scene.Lighting.PointLights);
	parameters->SpotLights = builder.CreateSRV(resources.External.Scene.Lighting.SpotLights);
	parameters->RectLights = builder.CreateSRV(resources.External.Scene.Lighting.RectLights);
	parameters->RayTracingHitVertices = builder.CreateSRV(resources.External.Scene.RayTracing.Vertices);
	parameters->MorphTargetDeltas = builder.CreateSRV(resources.External.Scene.RayTracing.MorphTargetDeltas);
	parameters->SkinInfluences = builder.CreateSRV(resources.External.Scene.RayTracing.SkinInfluences);
	parameters->RayTracingHitIndices = builder.CreateSRV(resources.External.Scene.RayTracing.Indices);
	parameters->RayTracingHitInstances = builder.CreateSRV(resources.External.Scene.RayTracing.Instances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(resources.External.Scene.RayTracing.Materials);
	parameters->MeshInstances = builder.CreateSRV(resources.External.Scene.Geometry.MeshInstances);
	parameters->JointMatrices = builder.CreateSRV(resources.External.Scene.Geometry.JointMatrices);
	parameters->MorphWeights = builder.CreateSRV(resources.External.Scene.Geometry.MorphWeights);
	RegisterRayTracingLightingParameterSetups(builder, parameters);
	builder.AddPassParameterSetup(
	    [parameterFields]
	    {
		    const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
		    parameterFields->RestirIndirectConstants = RestirIndirectLightingUniformData{
		        .BounceCount = settings.BounceCount,
		        .NormalBias = settings.NormalBias,
		        .MaxDistance = settings.MaxDistance};
	    });
	builder.Dispatch<RestirIndirectResolvePass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
