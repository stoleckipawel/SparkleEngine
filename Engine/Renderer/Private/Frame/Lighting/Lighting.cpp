#include "../../PCH.h"
#include "Frame/Lighting/Lighting.h"

#include "Frame/Lighting/DirectLighting.h"
#include "Frame/Lighting/IndirectLighting.h"
#include "Frame/Lighting/LightingComposite.h"
#include "Frame/Lighting/LightingRenderTargets.h"
#include "Frame/Lighting/LightingTargetClear.h"
#include "Frame/Lighting/Sky.h"

void AddLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	resources.Transient.Lighting = CreateLightingRenderTargets(builder, sceneExtent);

	AddLightingTargetClearPass(builder, resources.Transient.Lighting);
	AddDirectLightingPass(builder, resources.Transient.Lighting, resources.Transient.GBuffer, resources.Persistent.SceneTlas);
	AddIndirectLightingPasses(builder, resources.Transient.Lighting, resources.Transient.GBuffer, resources.Persistent.SceneTlas);
	AddLightingCompositePass(builder, resources.Transient.Scene, resources.Transient.Lighting, resources.Transient.GBuffer);
	AddSkyPass(builder, resources.Transient.Scene, resources.Transient.GBuffer);
}
