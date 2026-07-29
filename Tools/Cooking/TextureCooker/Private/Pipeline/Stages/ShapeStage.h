#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

namespace TextureCookPipeline
{
	void ApplyShapePolicy(const TextureCookRequest& request, WorkingTexture& workingTexture);
}
