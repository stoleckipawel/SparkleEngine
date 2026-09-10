#include "../../PCH.h"
#include "Passes/Lighting/Lighting.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Debug/RendererCVars.h"
#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Passes/Lighting/LightingComposite.h"
#include "Passes/Lighting/LightingRenderTargets.h"
#include "Passes/Lighting/LightingTargetClear.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerSample.h"
#include "Passes/Lighting/Restir/RestirLighting.h"
#include "Passes/Lighting/Restir/RestirRayReconstruction.h"
#include "Passes/Lighting/Sky/Sky.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RHI/Public/Formats/PixelFormat.h"

void AddLightingPasses(
    FrameGraphBuilder& builder,
    RenderRayTracingScene& rayTracingScene,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	const LightingMode mode = CVarLightingMode.Get();
	if (mode != LightingMode::RestirPathTraced && mode != LightingMode::ReferencePathTracer)
	{
		throw Diagnostics::Error("Lighting graph construction received an invalid lighting mode.");
	}
	const PixelFormat radianceFormat =
	    mode == LightingMode::ReferencePathTracer ? PixelFormat::R32G32B32A32_Float : RenderFrameGraphFormats::SceneColor;
	const bool createRayReconstructionGuides = mode == LightingMode::RestirPathTraced && IsRayReconstructionEnabled();
	resources.Transient.Lighting = CreateLightingRenderTargets(builder, sceneExtent, radianceFormat, createRayReconstructionGuides);
	AddLightingTargetClearPass(builder, resources.Transient.Lighting);

	switch (mode)
	{
		case LightingMode::RestirPathTraced:
			AddRestirLightingProducerPasses(builder, rayTracingScene, sceneExtent, resources);
			break;
		case LightingMode::ReferencePathTracer:
			AddReferencePathTracerProducerPasses(builder, sceneExtent, resources);
			break;
		default:
			break;
	}

	const FrameGraphTextureHandle lightingSample = mode == LightingMode::ReferencePathTracer
	    ? CreateReferencePathTracerSample(builder, sceneExtent)
	    : resources.Transient.Scene.SceneColor;
	AddLightingCompositePass(builder, sceneExtent, lightingSample, resources.Transient.Lighting, resources.Transient.GBuffer);
	AddSkyPass(builder, sceneExtent, lightingSample, resources.Transient.Scene.SceneDepth, resources.ImportedScene.Sky);

	switch (mode)
	{
		case LightingMode::ReferencePathTracer:
			FinalizeReferencePathTracerPasses(builder, sceneExtent, lightingSample, resources);
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
		case LightingMode::ReferencePathTracer:
			break;
		default:
			throw Diagnostics::Error("Lighting reconstruction received an invalid lighting mode.");
	}
}
