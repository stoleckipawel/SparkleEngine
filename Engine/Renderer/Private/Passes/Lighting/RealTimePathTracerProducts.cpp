#include "../../PCH.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"

bool HasRequiredRealTimePathTracerProducts(const RenderFrameGraphSettings& settings) noexcept
{
	return !settings.UseRayReconstruction || IsIndirectSpecularEnabled();
}

bool PrepareRealTimePathTracerProducts(const RenderFrameGraphSettings& settings, ViewportFrameProducts& products) noexcept
{
	if (HasRequiredRealTimePathTracerProducts(settings))
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
