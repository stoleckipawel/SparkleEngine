#include "../../PCH.h"
#include "Frame/Lighting/PathTracedDirectLighting.h"

#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Lighting/LightingRayTracingPasses.h"
#include "Passes/RayTracing/PathTracedDirectLightingPass.h"

void AddPathTracedDirectLightingPass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocPassParameters<PathTracedDirectLightingPass>();
	PathTracedDirectLightingPass::DeclareResources(
	    builder,
	    resources.Transient.Lighting,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.SceneTlas,
	    parameters);
	LightingRayTracingPasses::AddSceneTlasComputePass<PathTracedDirectLightingPass>(
	    builder,
	    parameters,
	    RayTracingSceneTlasShaderAccessMode::Descriptor);
}
