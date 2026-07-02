#include "../../PCH.h"
#include "Frame/Lighting/DirectLighting.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightingPass.h"

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals)
{
	auto& parameters = builder.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, shadowSignals, parameters);
	builder.AddComputeShaderPass<DirectLightingPass>(parameters);
}
