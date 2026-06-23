#include "../../PCH.h"
#include "Frame/Lighting/IndirectLighting.h"

#include "Frame/Lighting/IndirectDiffuse.h"
#include "Frame/Lighting/IndirectSpecular.h"

void AddIndirectLightingPasses(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	AddIndirectDiffusePass(builder, lighting, gbuffer, sceneTlas);
	AddIndirectSpecularPass(builder, lighting, gbuffer, sceneTlas);
}
