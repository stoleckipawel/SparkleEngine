#include "../../PCH.h"
#include "Frame/Lighting/LightingRenderTargets.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

class LightingRenderTargetFactory final
{
  public:
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
};

LightingRenderTargets CreateLightingRenderTargets(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat radianceFormat,
    bool createRayReconstructionGuides)
{
	LightingRenderTargets lighting{};
	lighting.DirectDiffuse = LightingRenderTargetFactory::CreateLightingTexture(builder, "DirectDiffuse", sceneExtent, radianceFormat);
	lighting.DirectSpecular = LightingRenderTargetFactory::CreateLightingTexture(builder, "DirectSpecular", sceneExtent, radianceFormat);
	lighting.DirectSubsurface = LightingRenderTargetFactory::CreateLightingTexture(builder, "DirectSubsurface", sceneExtent, radianceFormat);
	lighting.IndirectDiffuse = LightingRenderTargetFactory::CreateLightingTexture(builder, "IndirectDiffuse", sceneExtent, radianceFormat);
	lighting.IndirectSpecular = LightingRenderTargetFactory::CreateLightingTexture(builder, "IndirectSpecular", sceneExtent, radianceFormat);
	const RenderViewportExtent guideExtent = createRayReconstructionGuides ? sceneExtent : RenderViewportExtent{1u, 1u};
	lighting.ReconstructionGuides.DiffuseAlbedo =
	    LightingRenderTargetFactory::CreateLightingTexture(builder, "RayReconstructionDiffuseAlbedo", guideExtent, PixelFormat::R16G16B16A16_Float);
	lighting.ReconstructionGuides.SpecularAlbedo =
	    LightingRenderTargetFactory::CreateLightingTexture(builder, "RayReconstructionSpecularAlbedo", guideExtent, PixelFormat::R16G16B16A16_Float);
	lighting.ReconstructionGuides.Roughness =
	    LightingRenderTargetFactory::CreateLightingTexture(builder, "RayReconstructionRoughness", guideExtent, PixelFormat::R32_Float);
	lighting.ReconstructionGuides.SpecularHitDistance =
	    LightingRenderTargetFactory::CreateLightingTexture(builder, "RayReconstructionSpecularHitDistance", guideExtent, PixelFormat::R32_Float);
	return lighting;
}
