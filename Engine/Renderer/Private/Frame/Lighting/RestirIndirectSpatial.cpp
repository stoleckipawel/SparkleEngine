#include "../../PCH.h"
#include "Frame/Lighting/RestirIndirectSpatial.h"

#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RestirIndirectSpatialPass.h"

void AddRestirIndirectSpatialPass(
    FrameGraphBuilder& builder,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocPassParameters<RestirIndirectSpatialPass>();
	RestirIndirectSpatialPass::DeclareResources(
	    builder,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    workingReservoirs.TemporalSample,
	    workingReservoirs.TemporalWeight,
	    resources.History.RestirIndirectReservoir.Sample.Current,
	    resources.History.RestirIndirectReservoir.Weight.Current,
	    resources.History.RestirIndirectReservoir.Surface.Current,
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
	LightingRayTracingPasses::AddSceneTlasComputePass<RestirIndirectSpatialPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
