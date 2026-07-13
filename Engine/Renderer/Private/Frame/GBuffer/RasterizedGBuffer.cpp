#include "PCH.h"
#include "Frame/GBuffer/RasterizedGBuffer.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Core/FrameAssembly.h"
#include "Passes/Deferred/GBufferPass.h"

void AddRasterizedGBufferPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& targets,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& parameters = builder.AllocPassParameters<GBufferPass>();
	GBufferPass::DeclareResources(
	    builder,
	    targets,
	    externalResources.MeshInstances,
	    externalResources.JointMatrices,
	    externalResources.PreviousJointMatrices,
	    parameters);
	builder.AddRasterShaderPass<GBufferPass>(parameters);
}
