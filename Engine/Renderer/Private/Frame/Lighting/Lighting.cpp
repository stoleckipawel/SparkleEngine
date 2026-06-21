#include "../../PCH.h"
#include "Frame/Lighting/Lighting.h"

#include "Frame/Lighting/DirectLighting.h"
#include "Frame/Lighting/IndirectLighting.h"
#include "Frame/Lighting/LightingComposite.h"
#include "Frame/Lighting/IndirectSpecular.h"
#include "Frame/Lighting/Sky.h"
#include "Frame/Debug/VisualizeBuffers.h"

void AddLightingPasses(
    FrameGraphBuilder& builder,
    const SceneRenderTargets& sceneTargets,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	AddDirectLightingPass(builder, lighting, gbuffer, sceneTlas);
	AddIndirectLightingPass(builder, lighting, gbuffer);
	AddIndirectSpecularPass(builder, lighting, gbuffer, sceneTlas);
	AddLightingCompositePass(builder, sceneTargets, lighting, gbuffer);
	AddVisualizeBuffersPass(builder, sceneTargets, lighting, gbuffer);
	AddSkyPass(builder, sceneTargets, gbuffer);
}
