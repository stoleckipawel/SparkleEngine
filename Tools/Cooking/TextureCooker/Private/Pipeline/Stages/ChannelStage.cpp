#include "PCH.h"

#include "Pipeline/Stages/ChannelStage.h"

#include "Pipeline/ImageOps.h"

namespace TextureCookPipeline
{
	void ApplyChannelPolicy(const TextureCookRequest& request, WorkingTexture& workingTexture)
	{
		if (request.policy.channelMask == TextureChannelMask::Rgba)
		{
			return;
		}

		ExtractChannel(workingTexture, request.policy.channelMask);
	}
}
