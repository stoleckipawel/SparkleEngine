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
		    .DenoisedVisibility = FrameGraphTextureHandle::Invalid()};
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

		result.PackedSignal = builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(
		        "ShadowVisibilitySignalRaw",
		        sceneExtent.Width,
		        sceneExtent.Height,
		        ShadowDenoiseContract::PackedVisibilitySignalFormat));
		if (!requestDenoiser)
		{
			return result;
		}

		result.PackedSignalScratch = builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(
		        "ShadowVisibilitySignalScratch",
		        sceneExtent.Width,
		        sceneExtent.Height,
		        ShadowDenoiseContract::PackedVisibilitySignalFormat));

		result.DenoisedVisibility = builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(
		        "ShadowVisibilityDenoised",
		        sceneExtent.Width,
		        sceneExtent.Height,
		        ShadowDenoiseContract::DenoisedVisibilityFormat));

		return result;
	}
}  // namespace FrameGraphDenoiserRegistration
