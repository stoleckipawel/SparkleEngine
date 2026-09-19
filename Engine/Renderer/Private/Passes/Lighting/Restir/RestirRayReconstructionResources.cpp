#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirRayReconstructionResources.h"

#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"

RayReconstructionPassResources CreateRestirRayReconstructionResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    const RenderFrameGraphResources& resources)
{
	const FrameGraphTextureHandle denoisedSceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "DenoisedSceneColor",
	        renderExtent.Width,
	        renderExtent.Height,
	        RenderFrameGraphFormats::SceneColor));

	return RayReconstructionPassResources{
	    .NoisyInputColor = resources.Transient.Scene.SceneColor,
	    .OutputColor = denoisedSceneColor,
	    .Depth = resources.Transient.GBuffer.DeviceZ,
	    .MotionVectors = resources.Transient.GBuffer.MotionVector,
	    .Exposure = resources.Transient.Exposure,
	    .Normals = resources.Transient.GBuffer.Normal,
	    .Roughness = resources.Transient.Lighting.ReconstructionGuides.Roughness,
	    .DiffuseAlbedo = resources.Transient.Lighting.ReconstructionGuides.DiffuseAlbedo,
	    .SpecularAlbedo = resources.Transient.Lighting.ReconstructionGuides.SpecularAlbedo,
	    .SpecularHitDistance = resources.Transient.Lighting.ReconstructionGuides.SpecularHitDistance};
}
