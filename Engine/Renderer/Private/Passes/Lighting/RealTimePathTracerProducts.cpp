#include "../../PCH.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"

bool PrepareRealTimePathTracerProducts(RenderViewMode viewMode, ViewportFrameProducts& products) noexcept
{
	if (!ShouldUseRayReconstruction(viewMode) || CVarIndirectSpecular.Get())
	{
		return true;
	}
	products = {};
	products.Progress.State = ViewportRenderProgressState::Unavailable;
	products.Progress.Reason = ViewportRenderProgressReason::MissingRequiredProduct;
	return false;
}

void PublishRealTimePathTracerProducts(RenderFrameGraphResources& resources)
{
	resources.ViewportProducts.Radiance = resources.Transient.Scene.SceneColor;
}
