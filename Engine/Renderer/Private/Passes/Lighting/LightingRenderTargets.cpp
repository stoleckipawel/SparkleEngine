#include "../../PCH.h"
#include "Passes/Lighting/LightingRenderTargets.h"

#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

static FrameGraphTextureHandle CreateLightingTexture(
    FrameGraphBuilder& builder,
    const char* name,
    RenderViewportExtent sceneExtent,
    PixelFormat format)
{
	FrameGraphTextureDesc desc = FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, format);
	desc.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
	return builder.CreateTexture(desc);
}

void CreateRealTimeLightingRenderTargets(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool createRayReconstructionGuides,
    RenderFrameGraphResources& resources)
{
	LightingRenderTargets& lighting = resources.Transient.Lighting;
	const PixelFormat radianceFormat = RenderFrameGraphFormats::SceneColor;
	lighting.DirectDiffuse = CreateLightingTexture(builder, "DirectDiffuse", sceneExtent, radianceFormat);
	lighting.DirectSpecular = CreateLightingTexture(builder, "DirectSpecular", sceneExtent, radianceFormat);
	lighting.DirectSubsurface = CreateLightingTexture(builder, "DirectSubsurface", sceneExtent, radianceFormat);
	lighting.IndirectDiffuse = CreateLightingTexture(builder, "IndirectDiffuse", sceneExtent, radianceFormat);
	lighting.IndirectSpecular = CreateLightingTexture(builder, "IndirectSpecular", sceneExtent, radianceFormat);

	const RenderViewportExtent guideExtent = createRayReconstructionGuides ? sceneExtent : RenderViewportExtent{1u, 1u};
	lighting.ReconstructionGuides.DiffuseAlbedo =
	    CreateLightingTexture(builder, "RayReconstructionDiffuseAlbedo", guideExtent, PixelFormat::R16G16B16A16_Float);
	lighting.ReconstructionGuides.SpecularAlbedo =
	    CreateLightingTexture(builder, "RayReconstructionSpecularAlbedo", guideExtent, PixelFormat::R16G16B16A16_Float);
	lighting.ReconstructionGuides.Roughness =
	    CreateLightingTexture(builder, "RayReconstructionRoughness", guideExtent, PixelFormat::R32_Float);
	lighting.ReconstructionGuides.SpecularHitDistance =
	    CreateLightingTexture(builder, "RayReconstructionSpecularHitDistance", guideExtent, PixelFormat::R32_Float);
}
