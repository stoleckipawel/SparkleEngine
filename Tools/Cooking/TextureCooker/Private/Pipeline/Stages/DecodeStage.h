#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

#include <string>

namespace TextureCookPipeline
{
	bool BuildWorkingTexture(
	    const TextureCookRequest& request,
	    const TextureLoadResult& sourceTexture,
	    WorkingTexture& outWorkingTexture,
	    std::string& outErrorMessage);
}