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
	    resources.History.CurrentRestirIndirectReservoirSample,
	    resources.History.CurrentRestirIndirectReservoirWeight,
	    resources.History.CurrentRestirIndirectReservoirSurface,
	    resources.SceneTlas,
	    parameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<RestirIndirectSpatialPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
