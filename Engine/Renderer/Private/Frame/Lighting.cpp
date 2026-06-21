#include "../PCH.h"
#include "Frame/Lighting.h"

#include "Frame/DirectLighting.h"
#include "Frame/IndirectLighting.h"
#include "Frame/LightingComposite.h"
#include "Frame/RTIndirectSpecular.h"
#include "Frame/Sky.h"
#include "Frame/VisualizeBuffers.h"

void AddLightingPasses(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	AddDirectLightingPass(builder, lighting, gbuffer, sceneTlas);
	AddIndirectLightingPass(builder, lighting, gbuffer);
	AddRTIndirectSpecularPass(builder, lighting, gbuffer, sceneTlas);
	AddLightingCompositePass(builder, sceneTargets, lighting, gbuffer);
	AddVisualizeBuffersPass(builder, sceneTargets, lighting, gbuffer);
	AddSkyPass(builder, sceneTargets, gbuffer);
}
