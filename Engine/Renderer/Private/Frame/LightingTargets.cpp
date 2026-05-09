#include "../PCH.h"
#include "Frame/LightingTargets.h"

#include "Config/RenderConfig.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

namespace
{
	TextureHandle CreateLightingTexture(FrameGraph& frameGraph, const char* name, RenderViewportExtent sceneExtent)
	{
		return frameGraph.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, RenderConfig::SceneColorFormat));
	}
}

LightingTargets CreateLightingTargets(FrameGraph& frameGraph, RenderViewportExtent sceneExtent)
{
	LightingTargets lighting{};
	lighting.DirectDiffuse = CreateLightingTexture(frameGraph, "DirectDiffuse", sceneExtent);
	lighting.DirectSpecular = CreateLightingTexture(frameGraph, "DirectSpecular", sceneExtent);
	lighting.DirectSubsurface = CreateLightingTexture(frameGraph, "DirectSubsurface", sceneExtent);
	lighting.IndirectDiffuse = CreateLightingTexture(frameGraph, "IndirectDiffuse", sceneExtent);
	lighting.IndirectSpecular = CreateLightingTexture(frameGraph, "IndirectSpecular", sceneExtent);
	lighting.IndirectSubsurface = CreateLightingTexture(frameGraph, "IndirectSubsurface", sceneExtent);
	return lighting;
}