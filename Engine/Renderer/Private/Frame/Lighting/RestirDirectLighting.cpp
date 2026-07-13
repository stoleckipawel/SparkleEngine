#include "../../PCH.h"
#include "Frame/Lighting/RestirDirectLighting.h"

#include "Frame/Lighting/DirectLightReservoir.h"
#include "Frame/Lighting/DirectLighting.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "Frame/Lighting/Shadows/DirectShadowSignal.h"

void AddRestirDirectLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	const DirectShadowSignalResources shadowSignals = CreateDirectShadowSignalResources(builder, sceneExtent, resources);
	AddDirectLightReservoirPasses(builder, resources.Transient.Scene, resources.Transient.GBuffer, shadowSignals, resources.External);
	AddDirectShadowSignalPass(
	    builder,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.SceneTlas,
	    shadowSignals,
	    resources.External);
	AddDirectLightingPass(
	    builder,
	    resources.Transient.Lighting,
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    shadowSignals,
	    resources.External);
}
