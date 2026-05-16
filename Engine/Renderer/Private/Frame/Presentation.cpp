#include "../PCH.h"
#include "Frame/Presentation.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PassUtilities.h"

void AddPresentationPass(FrameGraphBuilder& builder, const SceneRenderTargets& sceneTargets)
{
	PassUtilities::AddCopyTexturePass(builder, "CopySceneColorToBackBuffer", sceneTargets.BackBuffer, sceneTargets.SceneColor);
}
