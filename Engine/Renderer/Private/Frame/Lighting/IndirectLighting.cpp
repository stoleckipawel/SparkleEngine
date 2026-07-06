#include "../../PCH.h"
#include "Frame/Lighting/IndirectLighting.h"

#include "Frame/Lighting/IndirectDiffuse.h"
#include "Frame/Lighting/IndirectSpecular.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseSettings.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSettings.h"

void AddIndirectLightingPasses(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	if (BuildIndirectDiffuseSettingsFromCVars().Enabled)
	{
		AddIndirectDiffusePass(builder, lighting, gbuffer, sceneTlas);
	}

	if (BuildIndirectSpecularSettingsFromCVars().Enabled)
	{
		AddIndirectSpecularPass(builder, lighting, gbuffer, sceneTlas);
	}
}
