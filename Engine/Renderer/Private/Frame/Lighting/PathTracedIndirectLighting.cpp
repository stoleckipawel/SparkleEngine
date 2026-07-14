#include "../../PCH.h"
#include "Frame/Lighting/PathTracedIndirectLighting.h"

#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "Passes/RayTracing/PathTracedIndirectLightingPass.h"

void AddPathTracedIndirectLightingPass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocParameters<PathTracedIndirectLightingPass::Parameters>();
	parameters->IndirectDiffuse = builder.CreateUAV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->IndirectSpecular = builder.CreateUAV(resources.Transient.Lighting.IndirectSpecular);
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
	LightingRayTracingPasses::DispatchSceneTlas<PathTracedIndirectLightingPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
