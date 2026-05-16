#include "../PCH.h"
#include "Frame/DirectLighting.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/DirectLightingPass.h"

void AddDirectLightingPass(FrameGraphBuilder& builder, const LightingTargets& lighting, const GBufferTargets& gbuffer)
{
	auto& parameters = builder.AllocPassParameters<DirectLightingPass>();
	DirectLightingPass::DeclareResources(builder, lighting, gbuffer, parameters);

	builder.AddComputePass<DirectLightingPass>(DirectLightingPass::PassName, parameters);
}
