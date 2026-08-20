#include "../../PCH.h"
#include "Frame/Lighting/RestirRayReconstruction.h"

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Presentation/LinearUpscaling.h"
#include "RayReconstruction/RayReconstructionPass.h"
#include "RayReconstruction/RayReconstructionSettings.h"

class RestirRayReconstructionInputBuilder final
{
public:
	static RayReconstructionPassResources BuildRayReconstructionInputs(
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    const LightingRenderTargets& lighting,
	    FrameGraphTextureHandle exposure)
	{
		return RayReconstructionPassResources{
		    .NoisyInputColor = sceneTargets.SceneColor,
		    .OutputColor = sceneTargets.FinalSceneColor,
		    .Depth = gbuffer.DeviceZ,
		    .MotionVectors = gbuffer.MotionVector,
		    .Exposure = exposure,
		    .Normals = gbuffer.Normal,
		    .Roughness = lighting.ReconstructionGuides.Roughness,
		    .DiffuseAlbedo = lighting.ReconstructionGuides.DiffuseAlbedo,
		    .SpecularAlbedo = lighting.ReconstructionGuides.SpecularAlbedo,
		    .SpecularHitDistance = lighting.ReconstructionGuides.SpecularHitDistance};
	}
};

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewportExtent outputExtent,
    IRayReconstructionProvider* rayReconstructionProvider,
    FrameAssemblyResourceLayout& resources)
{
	if (!IsRayReconstructionEnabled())
	{
		return;
	}

	const RayReconstructionPassResources providerInputs = RestirRayReconstructionInputBuilder::BuildRayReconstructionInputs(
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.Transient.Lighting,
	    resources.Transient.Exposure);

	AddLinearUpscalePass(builder, resources.Transient.Scene.SceneColor, resources.Transient.Scene.FinalSceneColor, outputExtent);
	if (rayReconstructionProvider != nullptr)
	{
		AddRayReconstructionPass(builder, *rayReconstructionProvider, "DlssRayReconstruction", sceneExtent, outputExtent, providerInputs);
	}
	// The linear pass is the guaranteed output path. The provider pass, when
	// available, overwrites it with the reconstructed result.
	resources.FinalSceneColorProduced = true;
}
