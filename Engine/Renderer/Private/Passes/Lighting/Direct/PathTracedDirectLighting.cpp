#include "../../../PCH.h"
#include "Passes/Lighting/Direct/PathTracedDirectLighting.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "ShaderData/RayTracingLightingParameters.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/PathTracedDirectLightingPass.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"

void AddPathTracedDirectLightingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<PathTracedDirectLightingPass::Parameters>();
	parameters->DirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(resources.Transient.Lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(resources.Transient.Lighting.DirectSubsurface);
	parameters->SceneTlas = builder.CreateAccelerationStructureBinding(resources.SceneTlas);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(resources.Transient.GBuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(resources.Transient.GBuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	parameters->DirectionalLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.PointLights);
	parameters->SpotLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.SpotLights);
	parameters->RectLights = builder.CreateSRV(resources.ImportedScene.Scene.Lighting.RectLights);
	parameters->RayTracingHitVertices = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Vertices);
	parameters->RayTracingHitIndices = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Indices);
	parameters->RayTracingHitInstances = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Instances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(resources.ImportedScene.Scene.RayTracing.Materials);
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
	builder.Dispatch<PathTracedDirectLightingPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
