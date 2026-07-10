#include "../../PCH.h"
#include "Frame/Lighting/Sky.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SkyPass.h"

void AddSkyPass(FrameGraphBuilder& builder, FrameGraphTextureHandle output, FrameGraphTextureHandle sceneDepth)
{
	auto& parameters = builder.AllocPassParameters<SkyPass>();
	SkyPass::DeclareResources(builder, output, sceneDepth, parameters);
	builder.AddComputeShaderPass<SkyPass>(parameters);
}
