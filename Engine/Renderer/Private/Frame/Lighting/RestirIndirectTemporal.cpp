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
	    resources.History.PreviousRestirIndirectReservoirSample,
	    resources.History.PreviousRestirIndirectReservoirWeight,
	    resources.History.PreviousRestirIndirectReservoirSurface,
	    resources.SceneTlas,
	    parameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<RestirIndirectTemporalPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
