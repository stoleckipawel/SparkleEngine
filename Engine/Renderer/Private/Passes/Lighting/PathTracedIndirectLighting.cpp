#include "../../PCH.h"
#include "Passes/Lighting/PathTracedIndirectLighting.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "ShaderData/RayTracingLightingParameters.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/PathTracedIndirectLightingPass.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"

void AddPathTracedIndirectLightingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<PathTracedIndirectLightingPass::Parameters>();
	parameters->IndirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(resources.Transient.Lighting.IndirectSpecular);
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
		    const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
		    fields.PathTracedLightingConstants = PathTracedLightingUniformData{
		        .SamplesPerPixel = settings.SamplesPerPixel,
		        .BounceCount = settings.BounceCount,
		        .NormalBias = settings.NormalBias,
		        .MaxDistance = settings.MaxDistance};
	    });
	builder.Dispatch<PathTracedIndirectLightingPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
