#include "../../PCH.h"
#include "Frame/Lighting/LightingRenderTargets.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

namespace
{
	FrameGraphTextureHandle CreateLightingTexture(
	    FrameGraphBuilder& builder,
	    const char* name,
	    RenderViewportExtent sceneExtent,
	    PixelFormat format)
	{
		FrameGraphTextureDesc desc =
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, format);
		desc.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
		return builder.CreateTexture(desc);
	}
}

LightingRenderTargets CreateLightingRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	LightingRenderTargets lighting{};
	lighting.DirectDiffuse = CreateLightingTexture(builder, "DirectDiffuse", sceneExtent, FrameRenderFormats::SceneColor);
	lighting.DirectSpecular = CreateLightingTexture(builder, "DirectSpecular", sceneExtent, FrameRenderFormats::SceneColor);
	lighting.DirectSubsurface = CreateLightingTexture(builder, "DirectSubsurface", sceneExtent, FrameRenderFormats::SceneColor);
	lighting.IndirectDiffuse = CreateLightingTexture(builder, "IndirectDiffuse", sceneExtent, FrameRenderFormats::SceneColor);
	lighting.IndirectSpecular = CreateLightingTexture(builder, "IndirectSpecular", sceneExtent, FrameRenderFormats::SceneColor);
	lighting.IndirectSubsurface = CreateLightingTexture(builder, "IndirectSubsurface", sceneExtent, FrameRenderFormats::SceneColor);
	lighting.IndirectDiffuseDemodulatedRadiance =
	    CreateLightingTexture(builder, "IndirectDiffuseDemodulatedRadiance", sceneExtent, PixelFormat::R32G32B32A32_Float);
	lighting.IndirectSpecularDemodulatedRadiance =
	    CreateLightingTexture(builder, "IndirectSpecularDemodulatedRadiance", sceneExtent, PixelFormat::R32G32B32A32_Float);
	lighting.IndirectDiffuseAlbedo =
	    CreateLightingTexture(builder, "IndirectDiffuseAlbedo", sceneExtent, PixelFormat::R32G32B32A32_Float);
	lighting.IndirectSpecularAlbedo =
	    CreateLightingTexture(builder, "IndirectSpecularAlbedo", sceneExtent, PixelFormat::R32G32B32A32_Float);
	lighting.IndirectMaterialGuide =
	    CreateLightingTexture(builder, "IndirectMaterialGuide", sceneExtent, PixelFormat::R32G32B32A32_Float);
	lighting.IndirectDiffuseSampleGuide =
	    CreateLightingTexture(builder, "IndirectDiffuseSampleGuide", sceneExtent, PixelFormat::R32G32B32A32_Float);
	lighting.IndirectSpecularSampleGuide =
	    CreateLightingTexture(builder, "IndirectSpecularSampleGuide", sceneExtent, PixelFormat::R32G32B32A32_Float);
	return lighting;
}
