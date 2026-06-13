#pragma once

#include "Denoising/ShadowDenoiseContract.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Viewport/ViewportContracts.h"

class FrameGraphBuilder;

namespace FrameGraphDenoiserRegistration
{
	ShadowDenoiseContract::ShadowDenoiseTextures RegisterShadowVisibilityResources(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent,
	    bool requestDenoiser);
}  // namespace FrameGraphDenoiserRegistration
