#include "../../PCH.h"
#include "Frame/Lighting/Lighting.h"

#include "Frame/Lighting/DirectLightReservoir.h"
#include "Frame/Lighting/DirectLighting.h"
#include "Frame/Lighting/IndirectLighting.h"
#include "Frame/Lighting/IndirectReconstruction.h"
#include "Frame/Lighting/LightingComposite.h"
#include "Frame/Lighting/LightingRenderTargets.h"
#include "Frame/Lighting/LightingTargetClear.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "Frame/Lighting/Shadows/DirectShadowSignal.h"
#include "Frame/Lighting/Sky.h"
#include "Renderer/Public/Debug/RendererCVars.h"

void AddLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	switch (CVarLightingMode.Get())
	{
	case LightingMode::Raytraced:
		break;
	case LightingMode::PathTraced:
		// Stage 5 will add the path-traced lighting producer.
		return;
	}

	resources.Transient.Lighting = CreateLightingRenderTargets(builder, sceneExtent);
	const DirectShadowSignalResources rawShadowSignals =
	    CreateDirectShadowSignalResources(builder, sceneExtent, resources);

	AddLightingTargetClearPass(builder, resources.Transient.Lighting);
	AddDirectLightReservoirPasses(
	    builder,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    rawShadowSignals);
	AddDirectShadowSignalPass(
	    builder,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.SceneTlas,
	    rawShadowSignals);
	AddDirectLightingPass(
	    builder,
	    resources.Transient.Lighting,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    rawShadowSignals);
	AddIndirectLightingPasses(
	    builder,
	    resources.Transient.Lighting,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.SceneTlas);
	AddLightingCompositePass(builder, resources.Transient.Scene, resources.Transient.Lighting, resources.Transient.GBuffer);
	AddSkyPass(builder, resources.Transient.Scene);
	AddIndirectRayReconstructionPassIfEnabled(builder, sceneExtent, resources);
}
