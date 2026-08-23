#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirDirectLighting.h"

#include "Passes/Lighting/Direct/DirectLightReservoir.h"
#include "Passes/Lighting/Direct/DirectLighting.h"
#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "Passes/Lighting/Shadows/DirectShadowSignal.h"

void AddRestirDirectLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	const DirectShadowSignalResources shadowSignals = CreateDirectShadowSignalResources(builder, sceneExtent, resources);
	AddDirectLightReservoirPasses(
	    builder,
	    sceneExtent,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    shadowSignals,
	    resources.ImportedScene);
	AddDirectShadowSignalPass(
	    builder,
	    sceneExtent,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.SceneTlas,
	    shadowSignals,
	    resources.ImportedScene);
	AddDirectLightingPass(
	    builder,
	    sceneExtent,
	    resources.Transient.Lighting,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    shadowSignals,
	    resources.ImportedScene);
}
