#include "../../PCH.h"
#include "Frame/Lighting/RestirIndirectTemporal.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectTemporalPass.h"

void AddRestirIndirectTemporalPass(
    FrameGraphBuilder& builder,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocPassParameters<RestirIndirectTemporalPass>();
	RestirIndirectTemporalPass::DeclareResources(
	    builder,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    workingReservoirs.TemporalSample,
	    workingReservoirs.TemporalWeight,
	    resources.History.RestirIndirectReservoir.Sample.Previous,
	    resources.History.RestirIndirectReservoir.Weight.Previous,
	    resources.History.RestirIndirectReservoir.Surface.Previous,
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
	LightingRayTracingPasses::AddSceneTlasComputePass<RestirIndirectTemporalPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
