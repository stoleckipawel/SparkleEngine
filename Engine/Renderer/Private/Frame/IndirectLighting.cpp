#include "../PCH.h"
#include "Frame/IndirectLighting.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/IndirectLightingPass.h"

void AddIndirectLightingPass(FrameGraphBuilder& builder, const LightingTargets& lighting)
{
	auto& parameters = builder.AllocPassParameters<IndirectLightingPass>();
	IndirectLightingPass::DeclareResources(builder, lighting, parameters);
	builder.AddComputePass<IndirectLightingPass>(IndirectLightingPass::PassName, parameters);
}
