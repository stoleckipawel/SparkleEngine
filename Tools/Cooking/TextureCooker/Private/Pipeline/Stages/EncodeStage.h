#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

namespace TextureCookPipeline
{
	TextureLoadResult ProcessCompressedSource(const TextureCookRequest& request, TextureLoadResult sourceTexture);

	TextureLoadResult BuildOutputTexture(const TextureCookRequest& request, const WorkingTexture& workingTexture);
}
