#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

namespace TextureCookPipeline
{
	WorkingTexture BuildWorkingTexture(const TextureCookRequest& request, const TextureLoadResult& sourceTexture);
}
