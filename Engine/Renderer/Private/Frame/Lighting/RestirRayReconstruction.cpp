#include "../../PCH.h"
#include "Frame/Lighting/RestirRayReconstruction.h"

#include "Frame/Core/FrameAssembly.h"
#include "RayReconstruction/RayReconstructionSettings.h"

namespace
{
	FrameRayReconstructionProviderResources BuildProviderInputs(
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    const LightingRenderTargets& lighting,
	    FrameGraphTextureHandle exposure)
	{
		return FrameRayReconstructionProviderResources{
		    .NoisyInputColor = sceneTargets.SceneColor,
		    .OutputColor = sceneTargets.ReconstructedSceneColor,
		    .Depth = sceneTargets.SceneDepth,
		    .MotionVectors = gbuffer.MotionVector,
		    .Exposure = exposure,
		    .Normals = gbuffer.Normal,
		    .Roughness = lighting.IndirectMaterialGuide,
		    .DiffuseAlbedo = lighting.IndirectDiffuseAlbedo,
		    .SpecularAlbedo = lighting.IndirectSpecularAlbedo,
		    .SpecularHitDistance = lighting.IndirectSpecularSampleGuide};
	}
}

void ConfigureRestirRayReconstruction(FrameAssemblyResourceLayout& resources)
{
	if (CVarRayReconstructionMode.Get() != EngineRayReconstructionMode::NvidiaDlrr)
	{
		return;
	}

	resources.RayReconstructionProviderInputs = BuildProviderInputs(
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.Transient.Lighting,
	    resources.Transient.Exposure);
}
