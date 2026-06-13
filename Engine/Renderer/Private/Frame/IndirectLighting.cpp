#include "../PCH.h"
#include "Frame/IndirectLighting.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/IndirectLightingPass.h"

void AddIndirectLightingPass(FrameGraphBuilder& builder, const LightingRenderTargets& lighting, const GBufferRenderTargets& gbuffer)
{
	auto& parameters = builder.AllocPassParameters<IndirectLightingPass>();
	IndirectLightingPass::DeclareResources(builder, lighting, gbuffer, parameters);
	builder.AddComputeShaderPass<IndirectLightingPass>(parameters);
}
