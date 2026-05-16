#include "../PCH.h"
#include "Frame/Lighting.h"

#include "Frame/DirectLighting.h"
#include "Frame/IndirectLighting.h"
#include "Frame/LightingComposite.h"
#include "Frame/Sky.h"
#include "Frame/VisualizeBuffers.h"

void AddLightingPasses(FrameGraphBuilder& builder, const SceneTargets& sceneTargets, const LightingTargets& lighting, const GBufferTargets& gbuffer)
{
	AddDirectLightingPass(builder, lighting, gbuffer);
	AddIndirectLightingPass(builder, lighting);
	AddLightingCompositePass(builder, sceneTargets, lighting, gbuffer);
	AddVisualizeBuffersPass(builder, sceneTargets, lighting, gbuffer);
	AddSkyPass(builder, sceneTargets, gbuffer);
}
