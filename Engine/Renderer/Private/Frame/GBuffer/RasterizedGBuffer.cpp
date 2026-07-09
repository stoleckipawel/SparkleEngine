#include "PCH.h"
#include "Frame/GBuffer/RasterizedGBuffer.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/GBufferPass.h"

void AddRasterizedGBufferPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets)
{
	auto& parameters = builder.AllocPassParameters<GBufferPass>();
	GBufferPass::DeclareResources(builder, targets, parameters);
	builder.AddRasterShaderPass<GBufferPass>(parameters);
}
