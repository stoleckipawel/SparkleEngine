#include "PCH.h"
#include "Frame/GBuffer/RaytracedGBuffer.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RaytracedGBufferPass.h"

void AddRaytracedGBufferPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	auto& parameters = builder.AllocPassParameters<RaytracedGBufferPass>();
	RaytracedGBufferPass::DeclareResources(builder, targets, sceneTlas, parameters);
	builder.AddComputeShaderPass<RaytracedGBufferPass>(parameters);
}
