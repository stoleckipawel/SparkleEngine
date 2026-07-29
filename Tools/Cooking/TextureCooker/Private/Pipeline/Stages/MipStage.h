#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

namespace TextureCookPipeline
{
	void ApplyMipPolicy(const TextureCookRequest& request, WorkingTexture& workingTexture);
}
