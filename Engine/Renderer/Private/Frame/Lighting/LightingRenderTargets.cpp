#include "../../PCH.h"
#include "Frame/Lighting/LightingRenderTargets.h"

#include "Config/RenderConfig.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

namespace
{
	FrameGraphTextureHandle CreateLightingTexture(FrameGraphBuilder& builder, const char* name, RenderViewportExtent sceneExtent)
	{
		return builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, RenderConfig::SceneColorFormat));
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
