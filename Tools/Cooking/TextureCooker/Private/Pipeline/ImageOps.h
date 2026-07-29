#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

namespace TextureCookPipeline
{
	float KaiserKernel(float x, float scale, void* userData);
	float KaiserSupport(float scale, void* userData);
	void ExtractChannel(WorkingTexture& workingTexture, TextureChannelMask channelMask);
}
