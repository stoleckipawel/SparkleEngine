#include "PCH.h"

#include "FrameGraph/Resources/FrameGraphDenoiserRegistration.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"

namespace
{
	ShadowDenoiseContract::ShadowDenoiseTextures CreateEmptyDenoiseTextures() noexcept
	{
		return ShadowDenoiseContract::ShadowDenoiseTextures{
		    .PackedSignal = FrameGraphTextureHandle::Invalid(),
		    .PackedSignalScratch = FrameGraphTextureHandle::Invalid(),
		    .DenoisedVisibility = FrameGraphTextureHandle::Invalid(),
		    .DenoisedVisibilityHistory = FrameGraphTextureHandle::Invalid()};
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

		result.PackedSignal = builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(
		        "ShadowVisibilitySignalRaw",
		        sceneExtent.Width,
		        sceneExtent.Height,
		        ShadowDenoiseContract::PackedVisibilitySignalFormat));
		result.PackedSignalScratch = builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(
		        "ShadowVisibilitySignalScratch",
		        sceneExtent.Width,
		        sceneExtent.Height,
		        ShadowDenoiseContract::PackedVisibilitySignalFormat));
		result.DenoisedVisibilityHistory = builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(
		        "ShadowVisibilityDenoisedHistory",
		        sceneExtent.Width,
		        sceneExtent.Height,
		        ShadowDenoiseContract::DenoisedVisibilityFormat));

		result.DenoisedVisibility = builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(
		        "ShadowVisibilityDenoised",
		        sceneExtent.Width,
		        sceneExtent.Height,
		        ShadowDenoiseContract::DenoisedVisibilityFormat));

		return result;
	}
}  // namespace FrameGraphDenoiserRegistration
