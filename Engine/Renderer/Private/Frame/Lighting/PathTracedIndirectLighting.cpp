#include "../../PCH.h"
#include "Frame/Lighting/PathTracedIndirectLighting.h"

#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "Passes/RayTracing/PathTracedIndirectLightingPass.h"

void AddPathTracedIndirectLightingPass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocPassParameters<PathTracedIndirectLightingPass>();
	PathTracedIndirectLightingPass::DeclareResources(
	    builder,
	    resources.Transient.Lighting,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.SceneTlas,
	    resources.External.Sky,
	    resources.External.DirectionalLights,
	    resources.External.PointLights,
	    resources.External.SpotLights,
	    resources.External.RectLights,
	    resources.External.RayTracingHitVertices,
	    resources.External.RayTracingHitSkinInfluences,
	    resources.External.RayTracingHitIndices,
	    resources.External.RayTracingHitInstances,
	    resources.External.RayTracingHitMaterials,
	    resources.External.MeshInstances,
	    resources.External.JointMatrices,
	    parameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<PathTracedIndirectLightingPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
