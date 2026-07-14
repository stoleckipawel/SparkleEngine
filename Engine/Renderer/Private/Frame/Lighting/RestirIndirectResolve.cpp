#include "../../PCH.h"
#include "Frame/Lighting/RestirIndirectResolve.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectResolvePass.h"

void AddRestirIndirectResolvePass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocPassParameters<RestirIndirectResolvePass>();
	RestirIndirectResolvePass::DeclareResources(
	    builder,
	    resources.Transient.Lighting,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.History.RestirIndirectReservoir.Sample.Current,
	    resources.History.RestirIndirectReservoir.Weight.Current,
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
	LightingRayTracingPasses::AddSceneTlasComputePass<RestirIndirectResolvePass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
