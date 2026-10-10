#include "PCH.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstructionResources.h"

#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"

void CreateRayReconstructionGuideRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent renderExtent, RenderFrameGraphResources& resources)
{
	const auto createGuide = [&builder, renderExtent](const char* name, PixelFormat format)
	{
		FrameGraphTextureDesc desc = FrameGraphTextureDesc::CreateColor(name, renderExtent.Width, renderExtent.Height, format);
		desc.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
		return builder.CreateTexture(desc);
	};

	auto& guides = resources.Transient.Lighting.ReconstructionGuides;
	guides.DiffuseAlbedo = createGuide("RayReconstructionDiffuseAlbedo", PixelFormat::R16G16B16A16_Float);
	guides.SpecularAlbedo = createGuide("RayReconstructionSpecularAlbedo", PixelFormat::R16G16B16A16_Float);
	guides.Roughness = createGuide("RayReconstructionRoughness", PixelFormat::R32_Float);
	guides.SpecularHitDistance = createGuide("RayReconstructionSpecularHitDistance", PixelFormat::R32_Float);
}

RayReconstructionPassResources CreateRestirRayReconstructionResources(FrameGraphBuilder& builder, RenderViewportExtent renderExtent, const RenderFrameGraphResources& resources)
{
	const FrameGraphTextureHandle denoisedSceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("DenoisedSceneColor", renderExtent.Width, renderExtent.Height, RenderFrameGraphFormats::SceneColor));

	return RayReconstructionPassResources{
	    .NoisyInputColor = resources.Transient.Scene.SceneColor,
	    .OutputColor = denoisedSceneColor,
	    .Depth = resources.Transient.GBuffer.DeviceZ,
	    .MotionVectors = resources.Transient.GBuffer.MotionVector,
	    .Exposure = resources.Transient.Exposure,
	    .Normals = resources.Transient.GBuffer.WorldNormal,
	    .Roughness = resources.Transient.Lighting.ReconstructionGuides.Roughness,
	    .DiffuseAlbedo = resources.Transient.Lighting.ReconstructionGuides.DiffuseAlbedo,
	    .SpecularAlbedo = resources.Transient.Lighting.ReconstructionGuides.SpecularAlbedo,
	    .SpecularHitDistance = resources.Transient.Lighting.ReconstructionGuides.SpecularHitDistance};
}
