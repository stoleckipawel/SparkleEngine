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
	    resources.History.CurrentRestirIndirectReservoirSample,
	    resources.History.CurrentRestirIndirectReservoirWeight,
	    resources.SceneTlas,
	    parameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<RestirIndirectResolvePass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
