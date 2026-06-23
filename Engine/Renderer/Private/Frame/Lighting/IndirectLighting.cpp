#include "../../PCH.h"
#include "Frame/Lighting/IndirectLighting.h"

#include "Frame/Lighting/AmbientIndirectLighting.h"
#include "Frame/Lighting/IndirectSpecular.h"

void AddIndirectLightingPasses(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	AddAmbientIndirectLightingPass(builder, lighting, gbuffer);
	AddIndirectSpecularPass(builder, lighting, gbuffer, sceneTlas);
}
