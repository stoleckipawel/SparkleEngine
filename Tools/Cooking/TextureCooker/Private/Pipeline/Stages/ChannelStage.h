#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

namespace TextureCookPipeline
{
	void ApplyChannelPolicy(const TextureCookRequest& request, WorkingTexture& workingTexture);
}
