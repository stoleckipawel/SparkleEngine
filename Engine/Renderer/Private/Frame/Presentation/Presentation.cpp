#include "../../PCH.h"
#include "Frame/Presentation/Presentation.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/PresentScenePass.h"

void AddPresentationPass(FrameGraphBuilder& builder, const SceneRenderTargets& sceneTargets)
{
	auto& parameters = builder.AllocPassParameters<PresentScenePass>();
	PresentScenePass::DeclareResources(builder, sceneTargets, parameters);
	builder.AddRasterShaderPass<PresentScenePass>(parameters);
}
