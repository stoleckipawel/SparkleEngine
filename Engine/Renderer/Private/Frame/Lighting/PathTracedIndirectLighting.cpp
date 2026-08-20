#include "../../PCH.h"
#include "Frame/Lighting/PathTracedIndirectLighting.h"

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Lighting/RayTracingLightingParameters.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/PathTracedIndirectLightingPass.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"

void AddPathTracedIndirectLightingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocParameters<PathTracedIndirectLightingPass::Parameters>();
	auto* parameterFields = parameters.operator->();
	parameters->IndirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(resources.Transient.Lighting.IndirectSpecular);
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
		    const PathTracedLightingSettings settings = BuildPathTracedLightingSettings();
		    parameterFields->PathTracedLightingConstants = PathTracedLightingUniformData{
		        .SamplesPerPixel = settings.SamplesPerPixel,
		        .BounceCount = settings.BounceCount,
		        .NormalBias = settings.NormalBias,
		        .MaxDistance = settings.MaxDistance};
	    });
	builder.Dispatch<PathTracedIndirectLightingPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
