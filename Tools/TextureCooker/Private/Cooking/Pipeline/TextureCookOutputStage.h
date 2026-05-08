#pragma once

#include "Cooking/Pipeline/TextureCookPipelineTypes.h"
#include "TextureCookRequestList.h"

#include <string>

using AssetAuthoring::TextureCookRequest;

namespace TextureCookPipeline
{
	bool ProcessCompressedSource(
	    const TextureCookRequest& request,
	    TextureLoadResult&& sourceTexture,
	    TextureLoadResult& outProcessedTexture,
	    std::string& outErrorMessage);

	bool BuildOutputTexture(
	    const TextureCookRequest& request,
	    const WorkingImage& workingImage,
	    TextureLoadResult& outProcessedTexture,
	    std::string& outErrorMessage);
}
