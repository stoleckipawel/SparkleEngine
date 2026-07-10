#include "../../PCH.h"
#include "Frame/Lighting/IndirectLighting.h"

#include "Frame/Lighting/IndirectDiffuse.h"
#include "Frame/Lighting/IndirectSpecular.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseCVars.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularCVars.h"

void AddIndirectLightingPasses(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas)
{
	if (CVarIndirectDiffuseEnabled.Get())
	{
		AddIndirectDiffusePass(builder, lighting, sceneTargets, gbuffer, sceneTlas);
	}

	if (CVarIndirectSpecularEnabled.Get())
	{
		AddIndirectSpecularPass(builder, lighting, sceneTargets, gbuffer, sceneTlas);
	}
}
