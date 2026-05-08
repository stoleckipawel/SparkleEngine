#pragma once

#include "Cooking/Pipeline/TextureCookPipelineTypes.h"
#include "TextureCookRequestList.h"

#include <string>

using AssetAuthoring::TextureCookRequest;

namespace TextureCookPipeline
{
	bool BuildWorkingImage(
	    const TextureCookRequest& request,
	    const TextureLoadResult& sourceTexture,
	    WorkingImage& outWorkingImage,
	    std::string& outErrorMessage);
}