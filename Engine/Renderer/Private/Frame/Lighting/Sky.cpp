#include "../../PCH.h"
#include "Frame/Lighting/Sky.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/SkyPass.h"

void AddSkyPass(FrameGraphBuilder& builder, const SceneRenderTargets& sceneTargets)
{
	auto& parameters = builder.AllocPassParameters<SkyPass>();
	SkyPass::DeclareResources(builder, sceneTargets, parameters);
	builder.AddComputeShaderPass<SkyPass>(parameters);
}
