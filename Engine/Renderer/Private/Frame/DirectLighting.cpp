#include "../PCH.h"
#include "Frame/DirectLighting.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/DirectLightingPass.h"

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameters = builder.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, sceneTlas, parameters);
	builder.AddComputeShaderPass<DirectLightingPass>(parameters);
}
