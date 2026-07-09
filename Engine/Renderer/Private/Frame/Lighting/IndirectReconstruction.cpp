#include "../../PCH.h"
#include "Frame/Lighting/IndirectReconstruction.h"

#include "RayReconstruction/RayReconstructionFramePass.h"
#include "RayReconstruction/RayReconstructionSettings.h"

FrameRayReconstructionProviderResources BuildIndirectRayReconstructionProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const LightingRenderTargets& lighting,
    FrameGraphTextureHandle exposure)
{
	return FrameRayReconstructionProviderResources{
	    .NoisyInputColor = sceneTargets.SceneColor,
	    .OutputColor = sceneTargets.ReconstructedSceneColor,
	    .Depth = sceneTargets.MainDepth,
	    .MotionVectors = gbuffer.MotionVector,
	    .Exposure = exposure,
	    .Normals = gbuffer.Normal,
	    .Roughness = lighting.IndirectMaterialGuide,
	    .DiffuseAlbedo = lighting.IndirectDiffuseAlbedo,
	    .SpecularAlbedo = lighting.IndirectSpecularAlbedo,
	    .SpecularHitDistance = lighting.IndirectSpecularSampleGuide};
}

bool AddIndirectRayReconstructionPassIfEnabled(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources)
{
	if (CVarRayReconstructionMode.Get() != EngineRayReconstructionMode::NvidiaDlrr)
	{
		return false;
	}

	resources.RayReconstructionProviderInputs = BuildIndirectRayReconstructionProviderInputs(
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.Transient.Lighting,
	    resources.Transient.Exposure);
	if (!HasRequiredRayReconstructionProviderResources(resources.RayReconstructionProviderInputs))
	{
		return false;
	}

	AddRayReconstructionProviderPass(
	    builder,
	    "IndirectRayReconstruction",
	    sceneExtent,
	    resources.RayReconstructionProviderInputs);
	resources.ReconstructedSceneColorProduced = true;
	return true;
}
