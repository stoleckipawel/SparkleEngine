#include "../../PCH.h"
#include "Frame/Lighting/Lighting.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "Frame/Lighting/LightingComposite.h"
#include "Frame/Lighting/LightingRenderTargets.h"
#include "Frame/Lighting/LightingTargetClear.h"
#include "Frame/Lighting/ReferenceLighting.h"
#include "Frame/Lighting/ReferenceLightingSample.h"
#include "Frame/Lighting/RestirLighting.h"
#include "Frame/Lighting/RestirRayReconstruction.h"
#include "Frame/Lighting/Sky.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "RHI/Public/Formats/PixelFormat.h"

void AddLightingPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	const LightingMode lightingMode = GetLightingMode();
	const PixelFormat radianceFormat =
	    lightingMode == LightingMode::ReferencePathTraced ? PixelFormat::R32G32B32A32_Float : FrameRenderFormats::SceneColor;
	const bool createRayReconstructionGuides = lightingMode == LightingMode::RestirPathTraced && IsRayReconstructionEnabled();
	resources.Transient.Lighting = CreateLightingRenderTargets(builder, sceneExtent, radianceFormat, createRayReconstructionGuides);
	AddLightingTargetClearPass(builder, resources.Transient.Lighting);

	switch (lightingMode)
	{
		case LightingMode::RestirPathTraced:
		default:
			AddRestirLightingProducerPasses(builder, sceneExtent, resources);
			break;
		case LightingMode::ReferencePathTraced:
			AddReferenceLightingProducerPasses(builder, resources);
			break;
	}

	const FrameGraphTextureHandle lightingSample = lightingMode == LightingMode::ReferencePathTraced
	                                                   ? CreateReferenceLightingSample(builder, sceneExtent)
	                                                   : resources.Transient.Scene.SceneColor;
	AddLightingCompositePass(builder, lightingSample, resources.Transient.Lighting, resources.Transient.GBuffer);
	AddSkyPass(builder, lightingSample, resources.Transient.Scene.SceneDepth, resources.External.Sky);

	switch (lightingMode)
	{
		case LightingMode::ReferencePathTraced:
			FinalizeReferenceLightingPasses(builder, lightingSample, resources);
			break;
		case LightingMode::RestirPathTraced:
		default:
			break;
	}
}

void AddLightingReconstructionPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewportExtent outputExtent,
    FrameAssemblyResourceLayout& resources)
{
	if (GetLightingMode() == LightingMode::RestirPathTraced)
	{
		AddRestirRayReconstructionPass(builder, sceneExtent, outputExtent, resources);
	}
}
