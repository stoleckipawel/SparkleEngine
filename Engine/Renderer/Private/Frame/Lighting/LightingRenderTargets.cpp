#include "../../PCH.h"
#include "Frame/Lighting/LightingRenderTargets.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

class LightingRenderTargetsOperations final
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
	lighting.DirectDiffuse = LightingRenderTargetsOperations::CreateLightingTexture(builder, "DirectDiffuse", sceneExtent, radianceFormat);
	lighting.DirectSpecular = LightingRenderTargetsOperations::CreateLightingTexture(builder, "DirectSpecular", sceneExtent, radianceFormat);
	lighting.DirectSubsurface = LightingRenderTargetsOperations::CreateLightingTexture(builder, "DirectSubsurface", sceneExtent, radianceFormat);
	lighting.IndirectDiffuse = LightingRenderTargetsOperations::CreateLightingTexture(builder, "IndirectDiffuse", sceneExtent, radianceFormat);
	lighting.IndirectSpecular = LightingRenderTargetsOperations::CreateLightingTexture(builder, "IndirectSpecular", sceneExtent, radianceFormat);
	const RenderViewportExtent guideExtent = createRayReconstructionGuides ? sceneExtent : RenderViewportExtent{1u, 1u};
	lighting.ReconstructionGuides.DiffuseAlbedo =
	    LightingRenderTargetsOperations::CreateLightingTexture(builder, "RayReconstructionDiffuseAlbedo", guideExtent, PixelFormat::R16G16B16A16_Float);
	lighting.ReconstructionGuides.SpecularAlbedo =
	    LightingRenderTargetsOperations::CreateLightingTexture(builder, "RayReconstructionSpecularAlbedo", guideExtent, PixelFormat::R16G16B16A16_Float);
	lighting.ReconstructionGuides.Roughness =
	    LightingRenderTargetsOperations::CreateLightingTexture(builder, "RayReconstructionRoughness", guideExtent, PixelFormat::R32_Float);
	lighting.ReconstructionGuides.SpecularHitDistance =
	    LightingRenderTargetsOperations::CreateLightingTexture(builder, "RayReconstructionSpecularHitDistance", guideExtent, PixelFormat::R32_Float);
	return lighting;
}
