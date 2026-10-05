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

void CreateDirectLightingRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	LightingRenderTargets& lighting = resources.Transient.Lighting;
	const PixelFormat radianceFormat = RenderFrameGraphFormats::SceneColor;
	lighting.DirectDiffuse = CreateLightingTexture(builder, "DirectDiffuse", sceneExtent, radianceFormat);
	lighting.DirectSpecular = CreateLightingTexture(builder, "DirectSpecular", sceneExtent, radianceFormat);
	lighting.DirectSubsurface = CreateLightingTexture(builder, "DirectSubsurface", sceneExtent, radianceFormat);
}

void CreateIndirectLightingRenderTargets(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    bool createRayReconstructionGuides,
    RenderFrameGraphResources& resources)
{
	LightingRenderTargets& lighting = resources.Transient.Lighting;
	const PixelFormat radianceFormat = RenderFrameGraphFormats::SceneColor;
	lighting.IndirectDiffuse = CreateLightingTexture(builder, "IndirectDiffuse", sceneExtent, radianceFormat);
	lighting.IndirectSpecular = CreateLightingTexture(builder, "IndirectSpecular", sceneExtent, radianceFormat);

	if (!createRayReconstructionGuides)
	{
		return;
	}

	lighting.ReconstructionGuides.DiffuseAlbedo =
	    CreateLightingTexture(builder, "RayReconstructionDiffuseAlbedo", sceneExtent, PixelFormat::R16G16B16A16_Float);

	lighting.ReconstructionGuides.SpecularAlbedo =
	    CreateLightingTexture(builder, "RayReconstructionSpecularAlbedo", sceneExtent, PixelFormat::R16G16B16A16_Float);

	lighting.ReconstructionGuides.Roughness =
	    CreateLightingTexture(builder, "RayReconstructionRoughness", sceneExtent, PixelFormat::R32_Float);

	lighting.ReconstructionGuides.SpecularHitDistance =
	    CreateLightingTexture(builder, "RayReconstructionSpecularHitDistance", sceneExtent, PixelFormat::R32_Float);
}
