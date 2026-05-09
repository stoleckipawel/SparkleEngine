#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

#include <string>

namespace TextureCookPipeline
{
	bool ProcessCompressedSource(
	    const TextureCookRequest& request,
	    TextureLoadResult&& sourceTexture,
	    TextureLoadResult& outProcessedTexture,
	    std::string& outErrorMessage);

	bool BuildOutputTexture(
	    const TextureCookRequest& request,
	    const WorkingTexture& workingTexture,
	    TextureLoadResult& outProcessedTexture,
	    std::string& outErrorMessage);
}