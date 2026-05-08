#pragma once

#include "Cooking/Pipeline/TextureCookPipelineTypes.h"
#include "TextureCookRequestList.h"

#include <string>

using AssetAuthoring::TextureCookRequest;

namespace TextureCookPipeline
{
	bool ApplyMipPolicy(
	    const TextureCookRequest& request,
	    WorkingImage& workingImage,
	    std::string& outErrorMessage);
}