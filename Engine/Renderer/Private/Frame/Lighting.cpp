#include "../PCH.h"
#include "Frame/Lighting.h"

#include "Frame/DirectLighting.h"
#include "Frame/IndirectLighting.h"
#include "Frame/LightingComposite.h"
#include "Frame/Sky.h"
#include "Frame/VisualizeBuffers.h"

void AddLightingPasses(FrameGraph& frameGraph, const SceneTargets& sceneTargets, const LightingTargets& lighting, const GBufferTargets& gbuffer)
{
	AddDirectLightingPass(frameGraph, lighting, gbuffer);
	AddIndirectLightingPass(frameGraph, lighting);
	AddLightingCompositePass(frameGraph, sceneTargets, lighting, gbuffer);
	AddVisualizeBuffersPass(frameGraph, sceneTargets, lighting, gbuffer);
	AddSkyPass(frameGraph, sceneTargets, gbuffer);
}
