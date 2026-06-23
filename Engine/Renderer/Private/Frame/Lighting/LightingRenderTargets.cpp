#include "../../PCH.h"
#include "Frame/Lighting/LightingRenderTargets.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

namespace
{
	FrameGraphTextureHandle CreateLightingTexture(FrameGraphBuilder& builder, const char* name, RenderViewportExtent sceneExtent)
	{
		FrameGraphTextureDesc desc =
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, FrameRenderFormats::SceneColor);
		desc.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
		return builder.CreateTexture(desc);
	}
}

LightingRenderTargets CreateLightingRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	LightingRenderTargets lighting{};
	lighting.DirectDiffuse = CreateLightingTexture(builder, "DirectDiffuse", sceneExtent);
	lighting.DirectSpecular = CreateLightingTexture(builder, "DirectSpecular", sceneExtent);
	lighting.DirectSubsurface = CreateLightingTexture(builder, "DirectSubsurface", sceneExtent);
	lighting.IndirectDiffuse = CreateLightingTexture(builder, "IndirectDiffuse", sceneExtent);
	lighting.IndirectSpecular = CreateLightingTexture(builder, "IndirectSpecular", sceneExtent);
	lighting.IndirectSubsurface = CreateLightingTexture(builder, "IndirectSubsurface", sceneExtent);
	return lighting;
}
