#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

#include <string>

namespace TextureCookPipeline
{
	bool ApplyShapePolicy(const TextureCookRequest& request, WorkingTexture& workingTexture, std::string& outErrorMessage);
}