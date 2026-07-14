#include "../../PCH.h"
#include "Frame/Lighting/PathTracedDirectLighting.h"

#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "Passes/RayTracing/PathTracedDirectLightingPass.h"

void AddPathTracedDirectLightingPass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocParameters<PathTracedDirectLightingPass::Parameters>();
	parameters->DirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(resources.Transient.Lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(resources.Transient.Lighting.DirectSubsurface);
	parameters->SceneTlas = builder.Read(resources.SceneTlas);
	parameters->GBufferBaseColor = builder.CreateSRV(resources.Transient.GBuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(resources.Transient.GBuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(resources.Transient.GBuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(resources.Transient.GBuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	parameters->DirectionalLights = builder.CreateSRV(resources.External.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(resources.External.PointLights);
	parameters->SpotLights = builder.CreateSRV(resources.External.SpotLights);
	parameters->RectLights = builder.CreateSRV(resources.External.RectLights);
	parameters->RayTracingHitVertices = builder.CreateSRV(resources.External.RayTracingHitVertices);
	parameters->RayTracingHitIndices = builder.CreateSRV(resources.External.RayTracingHitIndices);
	parameters->RayTracingHitInstances = builder.CreateSRV(resources.External.RayTracingHitInstances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(resources.External.RayTracingHitMaterials);
	LightingRayTracingPasses::DispatchSceneTlas<PathTracedDirectLightingPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
