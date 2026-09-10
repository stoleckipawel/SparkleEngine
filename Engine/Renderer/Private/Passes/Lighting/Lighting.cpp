#include "../../PCH.h"
#include "Passes/Lighting/Lighting.h"

#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Passes/Lighting/LightingComposite.h"
#include "Passes/Lighting/LightingRenderTargets.h"
#include "Passes/Lighting/LightingTargetClear.h"
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
	resources.Transient.Lighting =
	    CreateLightingRenderTargets(builder, sceneExtent, RenderFrameGraphFormats::SceneColor, IsRayReconstructionEnabled());
	AddLightingTargetClearPass(builder, resources.Transient.Lighting);
	AddRestirLightingProducerPasses(builder, rayTracingScene, sceneExtent, resources);
	const FrameGraphTextureHandle lightingSample = resources.Transient.Scene.SceneColor;
	AddLightingCompositePass(builder, sceneExtent, lightingSample, resources.Transient.Lighting, resources.Transient.GBuffer);
	AddSkyPass(builder, sceneExtent, lightingSample, resources.Transient.Scene.SceneDepth, resources.ImportedScene.Sky);
}

void AddLightingReconstructionPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewportExtent outputExtent,
    IRayReconstructionProvider* rayReconstructionProvider,
    RenderFrameGraphResources& resources)
{
	AddRestirRayReconstructionPass(builder, sceneExtent, outputExtent, rayReconstructionProvider, resources);
}
