#include "../../PCH.h"
#include "Frame/Lighting/PathTracedDirectLighting.h"

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Lighting/RayTracingLightingParameters.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/PathTracedDirectLightingPass.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"

void AddPathTracedDirectLightingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocParameters<PathTracedDirectLightingPass::Parameters>();
	auto* parameterFields = parameters.operator->();
	parameters->DirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(resources.Transient.Lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(resources.Transient.Lighting.DirectSubsurface);
	parameters->SceneTlas = builder.Read(resources.SceneTlas);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(resources.Transient.GBuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(resources.Transient.GBuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	parameters->DirectionalLights = builder.CreateSRV(resources.External.Scene.Lighting.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(resources.External.Scene.Lighting.PointLights);
	parameters->SpotLights = builder.CreateSRV(resources.External.Scene.Lighting.SpotLights);
	parameters->RectLights = builder.CreateSRV(resources.External.Scene.Lighting.RectLights);
	parameters->RayTracingHitVertices = builder.CreateSRV(resources.External.Scene.RayTracing.Vertices);
	parameters->RayTracingHitIndices = builder.CreateSRV(resources.External.Scene.RayTracing.Indices);
	parameters->RayTracingHitInstances = builder.CreateSRV(resources.External.Scene.RayTracing.Instances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(resources.External.Scene.RayTracing.Materials);
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
	builder.Dispatch<PathTracedDirectLightingPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
