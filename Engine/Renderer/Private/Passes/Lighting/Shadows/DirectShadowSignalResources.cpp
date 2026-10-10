#include "../../../PCH.h"
#include "Passes/Lighting/Shadows/DirectShadowSignalResources.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

void CreateDirectShadowSignalResources(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	const FrameGraphTextureDesc visibilityDesc = FrameGraphTextureDesc::CreateColor("ShadowVisibilitySignalRaw", sceneExtent.Width, sceneExtent.Height, PixelFormat::R32G32B32A32_Float);
	resources.Transient.ShadowVisibilitySignal = builder.CreateTexture(visibilityDesc);
}
