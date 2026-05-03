#include "../PCH.h"
#include "Frame/Lighting.h"

#include "Frame/DirectLighting.h"

void BuildLighting(
    FrameGraph& frameGraph,
	const SceneTargets& sceneTargets,
	const GBufferTargets& gbuffer)
{
	BuildDirectLighting(frameGraph, sceneTargets, gbuffer);

	// TODO: BuildIndirectLighting(frameGraph, sceneTargets, gbuffer);
	// TODO: BuildVolumetrics(frameGraph, sceneTargets);
}
