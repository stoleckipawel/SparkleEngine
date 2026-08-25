#include "../../PCH.h"
#include "Passes/Lighting/Lighting.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Debug/RendererCVars.h"
#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Passes/Lighting/LightingComposite.h"
#include "Passes/Lighting/LightingRenderTargets.h"
#include "Passes/Lighting/LightingTargetClear.h"
#include "Passes/Lighting/Reference/ReferenceLighting.h"
#include "Passes/Lighting/Reference/ReferenceLightingSample.h"
#include "Passes/Lighting/Restir/RestirLighting.h"
#include "Passes/Lighting/Restir/RestirRayReconstruction.h"
#include "Passes/Lighting/Sky/Sky.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RHI/Public/Formats/PixelFormat.h"

void AddLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	const LightingMode mode = CVarLightingMode.Get();
	if (mode != LightingMode::RestirPathTraced && mode != LightingMode::ReferencePathTraced)
	{
		throw Diagnostics::Error("Lighting graph construction received an invalid lighting mode.");
	}
	const PixelFormat radianceFormat =
	    mode == LightingMode::ReferencePathTraced ? PixelFormat::R32G32B32A32_Float : RenderFrameGraphFormats::SceneColor;
	const bool createRayReconstructionGuides = mode == LightingMode::RestirPathTraced && IsRayReconstructionEnabled();
	resources.Transient.Lighting = CreateLightingRenderTargets(builder, sceneExtent, radianceFormat, createRayReconstructionGuides);
	AddLightingTargetClearPass(builder, resources.Transient.Lighting);

	switch (mode)
	{
		case LightingMode::RestirPathTraced:
			AddRestirLightingProducerPasses(builder, sceneExtent, resources);
			break;
		case LightingMode::ReferencePathTraced:
			AddReferenceLightingProducerPasses(builder, sceneExtent, resources);
			break;
		default:
			break;
	}

	const FrameGraphTextureHandle lightingSample = mode == LightingMode::ReferencePathTraced
	    ? CreateReferenceLightingSample(builder, sceneExtent)
	    : resources.Transient.Scene.SceneColor;
	AddLightingCompositePass(builder, sceneExtent, lightingSample, resources.Transient.Lighting, resources.Transient.GBuffer);
	AddSkyPass(builder, sceneExtent, lightingSample, resources.Transient.Scene.SceneDepth, resources.ImportedScene.Sky);

	switch (mode)
	{
		case LightingMode::ReferencePathTraced:
			FinalizeReferenceLightingPasses(builder, sceneExtent, lightingSample, resources);
			break;
		case LightingMode::RestirPathTraced:
			break;
		default:
			break;
	}
}

void AddLightingReconstructionPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewportExtent outputExtent,
    IRayReconstructionProvider* rayReconstructionProvider,
    RenderFrameGraphResources& resources)
{
	switch (CVarLightingMode.Get())
	{
		case LightingMode::RestirPathTraced:
			AddRestirRayReconstructionPass(builder, sceneExtent, outputExtent, rayReconstructionProvider, resources);
			break;
		case LightingMode::ReferencePathTraced:
			break;
		default:
			throw Diagnostics::Error("Lighting reconstruction received an invalid lighting mode.");
	}
}
