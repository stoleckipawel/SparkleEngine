#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirIndirectResolve.h"

#include "Core/Public/Math/MathUtils.h"
#include "ShaderData/RayTracingLightingParameters.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectResolveShader.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"

void AddRestirIndirectResolvePass(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<RestirIndirectResolveCS>();
	parameters->CurrentReservoirSampleTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Sample.Current);
	parameters->CurrentReservoirWeightTexture = builder.CreateSRV(resources.History.RestirIndirectReservoir.Weight.Current);
	parameters->IndirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(resources.Transient.Lighting.IndirectSpecular);
	parameters->RayReconstructionDiffuseAlbedo = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.DiffuseAlbedo);
	parameters->RayReconstructionSpecularAlbedo = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.SpecularAlbedo);
	parameters->RayReconstructionRoughness = builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.Roughness);
	parameters->RayReconstructionSpecularHitDistance =
	    builder.CreateUAV(resources.Transient.Lighting.ReconstructionGuides.SpecularHitDistance);
	parameters->SceneTlas = builder.CreateAccelerationStructureBinding(resources.SceneTlas);
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
	builder.Dispatch<RestirIndirectResolveCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
