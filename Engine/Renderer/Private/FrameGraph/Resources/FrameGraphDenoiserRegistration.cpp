#include "PCH.h"

#include "FrameGraph/Resources/FrameGraphDenoiserRegistration.h"

#include "Frame/Deferred/GBufferFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

namespace
{
	ShadowDenoiseContract::ShadowDenoiseTextures CreateEmptyDenoiseTextures() noexcept
	{
		return ShadowDenoiseContract::ShadowDenoiseTextures{
		    .RawVisibility = FrameGraphTextureHandle::Invalid(),
		    .RawVisibilityScratch = FrameGraphTextureHandle::Invalid(),
		    .DenoisedVisibility = FrameGraphTextureHandle::Invalid(),
		    .DenoiseHistory = FrameGraphTextureHandle::Invalid()};
	}
}

namespace FrameGraphDenoiserRegistration
{
	ShadowDenoiseContract::ShadowDenoiseTextures RegisterShadowVisibilityResources(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent,
	    bool requestDenoiser)
	{
		ShadowDenoiseContract::ShadowDenoiseTextures result = CreateEmptyDenoiseTextures();
		if (!requestDenoiser)
		{
			return result;
		}

		result.RawVisibility =
		    builder.CreateTexture(FrameGraphTextureDesc::CreateColor("ShadowVisibilityRaw", sceneExtent.Width, sceneExtent.Height, GBufferFormats::DeviceZ));
		result.RawVisibilityScratch =
		    builder.CreateTexture(FrameGraphTextureDesc::CreateColor("ShadowVisibilityScratch", sceneExtent.Width, sceneExtent.Height, GBufferFormats::DeviceZ));
		result.DenoiseHistory =
		    builder.CreateTexture(FrameGraphTextureDesc::CreateColor("ShadowDenoiseHistory", sceneExtent.Width, sceneExtent.Height, GBufferFormats::DeviceZ));

		result.DenoisedVisibility = builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor("ShadowVisibilityDenoised", sceneExtent.Width, sceneExtent.Height, GBufferFormats::DeviceZ));

		return result;
	}
}  // namespace FrameGraphDenoiserRegistration
