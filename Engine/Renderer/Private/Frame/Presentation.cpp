#include "../PCH.h"
#include "Frame/Presentation.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/PassUtilities.h"

void AddPresentationPass(FrameGraph& frameGraph, const SceneTargets& sceneTargets)
{
	PassUtilities::AddCopyTexturePass(frameGraph, "CopySceneColorToBackBuffer", sceneTargets.BackBuffer, sceneTargets.SceneColor);
}
