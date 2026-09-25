#include "PCH.h"
#include "Passes/Presentation/PresentationPolicy.h"

#include "Core/Public/Diagnostics/Error.h"

RenderViewPresentationDomain ResolveRenderViewPresentationDomain(RenderViewMode viewMode)
{
	switch (viewMode)
	{
		case RenderViewMode::Lit:
		case RenderViewMode::ReferencePathTracer:
		case RenderViewMode::Wireframe:
		case RenderViewMode::GBufferEmissive:
		case RenderViewMode::DirectDiffuse:
		case RenderViewMode::DirectSpecular:
		case RenderViewMode::DirectSubsurface:
		case RenderViewMode::IndirectDiffuse:
		case RenderViewMode::IndirectSpecular:
			return RenderViewPresentationDomain::SceneReferredHdr;
		case RenderViewMode::GBufferDiffuse:
		case RenderViewMode::GBufferNormal:
		case RenderViewMode::GBufferRoughness:
		case RenderViewMode::GBufferMetallic:
		case RenderViewMode::GBufferAmbientOcclusion:
		case RenderViewMode::GBufferSubsurfaceColor:
		case RenderViewMode::GBufferSubsurfaceStrength:
		case RenderViewMode::GpuSceneInstances:
			return RenderViewPresentationDomain::DisplayLinearExact;
		case RenderViewMode::Count:
		default:
			throw Diagnostics::Error("Presentation graph construction received an invalid view mode.");
	}
}

SceneUpscalingMethod ResolveSceneUpscalingMethod(RenderViewMode viewMode)
{
	if (viewMode == RenderViewMode::ReferencePathTracer)
	{
		return SceneUpscalingMethod::Linear;
	}

	return ResolveRenderViewPresentationDomain(viewMode) == RenderViewPresentationDomain::DisplayLinearExact
	    ? SceneUpscalingMethod::Point
	    : SceneUpscalingMethod::ConfiguredProvider;
}

RenderViewportExtent ResolveSceneRenderExtent(
    RenderViewMode viewMode,
    RenderViewportExtent outputExtent,
    RenderViewportExtent configuredRenderExtent)
{
	return ResolveSceneUpscalingMethod(viewMode) == SceneUpscalingMethod::ConfiguredProvider ? configuredRenderExtent : outputExtent;
}
