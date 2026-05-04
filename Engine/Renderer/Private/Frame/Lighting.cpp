#include "../PCH.h"
#include "Frame/Lighting.h"

#include "Frame/DirectLighting.h"
#include "Frame/IndirectLighting.h"
#include "Frame/LightingComposite.h"
#include "Frame/LightingTargets.h"
#include "Frame/Sky.h"
#include "Frame/VisualizeBuffers.h"

void BuildLighting(
    FrameGraph& frameGraph,
	RenderViewportExtent sceneExtent,
	const SceneTargets& sceneTargets,
	const GBufferTargets& gbuffer)
{
	const LightingTargets lighting = CreateLightingTargets(frameGraph, sceneExtent);

	BuildDirectLighting(frameGraph, lighting, gbuffer);
	BuildIndirectLighting(frameGraph, lighting);
	BuildLightingComposite(frameGraph, sceneTargets, lighting, gbuffer);
	BuildVisualizeBuffers(frameGraph, sceneTargets, lighting, gbuffer);
	BuildSky(frameGraph, sceneTargets, gbuffer);
}
