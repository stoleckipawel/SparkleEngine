#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

#include <string>

namespace TextureCookPipeline
{
	float KaiserKernel(float x, float scale, void* userData);
	float KaiserSupport(float scale, void* userData);
	bool ExtractChannel(WorkingTexture& workingTexture, TextureChannelMask channelMask, std::string& outErrorMessage);
}