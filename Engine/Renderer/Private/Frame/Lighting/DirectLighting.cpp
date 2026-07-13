#include "../../PCH.h"
#include "Frame/Lighting/DirectLighting.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightingPass.h"

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& parameters = builder.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(
	    builder,
	    lighting,
	    sceneTargets.SceneDepth,
	    gbuffer,
	    shadowSignals,
	    externalResources.DirectionalLights,
	    externalResources.PointLights,
	    externalResources.SpotLights,
	    externalResources.RectLights,
	    parameters);
	builder.AddComputeShaderPass<DirectLightingPass>(parameters);
}
