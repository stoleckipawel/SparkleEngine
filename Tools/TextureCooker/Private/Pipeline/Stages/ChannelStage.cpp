#include "PCH.h"

#include "Pipeline/Stages/ChannelStage.h"

#include "Pipeline/ImageOps.h"

namespace TextureCookPipeline
{
	bool ApplyChannelPolicy(const TextureCookRequest& request, WorkingTexture& workingTexture, std::string& outErrorMessage)
	{
		if (request.channelMask == TextureChannelMask::Rgba)
		{
			outErrorMessage.clear();
			return true;
		}

		if (!ExtractChannel(workingTexture, request.channelMask, outErrorMessage))
		{
			return false;
		}

		outErrorMessage.clear();
		return true;
	}
}