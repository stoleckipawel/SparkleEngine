#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirRayReconstruction.h"

#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Presentation/LinearUpscale.h"
#include "Providers/RendererImageProviderStack.h"
#include "RayReconstruction/RayReconstructionPass.h"
#include "RayReconstruction/RayReconstructionSettings.h"

static RayReconstructionPassResources BuildRestirRayReconstructionInputs(
    const SceneRenderTargets& sceneTargets,
    FrameGraphTextureHandle resolvedSceneColor,
    const GBufferRenderTargets& gbuffer,
    const LightingRenderTargets& lighting,
    FrameGraphTextureHandle exposure)
{
	return RayReconstructionPassResources{
	    .NoisyInputColor = sceneTargets.SceneColor,
	    .OutputColor = resolvedSceneColor,
	    .Depth = gbuffer.DeviceZ,
	    .MotionVectors = gbuffer.MotionVector,
	    .Exposure = exposure,
	    .Normals = gbuffer.Normal,
	    .Roughness = lighting.ReconstructionGuides.Roughness,
	    .DiffuseAlbedo = lighting.ReconstructionGuides.DiffuseAlbedo,
	    .SpecularAlbedo = lighting.ReconstructionGuides.SpecularAlbedo,
	    .SpecularHitDistance = lighting.ReconstructionGuides.SpecularHitDistance};
}

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	if (!IsRayReconstructionEnabled())
	{
		return;
	}

	resources.ResolvedSceneColor = CreateResolvedSceneColor(builder, settings.OutputExtent);
	const RayReconstructionPassResources providerInputs = BuildRestirRayReconstructionInputs(
	    resources.Transient.Scene,
	    resources.ResolvedSceneColor,
	    resources.Transient.GBuffer,
	    resources.Transient.Lighting,
	    resources.Transient.Exposure);

	AddLinearUpscalePass(builder, resources.Transient.Scene.SceneColor, resources.ResolvedSceneColor, settings.OutputExtent);
	IRayReconstructionProvider* rayReconstructionProvider = imageProviders.GetRayReconstructionProvider();
	if (rayReconstructionProvider != nullptr)
	{
		AddRayReconstructionPass(
		    builder,
		    *rayReconstructionProvider,
		    "DlssRayReconstruction",
		    settings.RenderExtent,
		    settings.OutputExtent,
		    providerInputs);
	}
	// The linear pass is the guaranteed output path. The provider pass, when
	// available, overwrites it with the reconstructed result.
}
