#pragma once

#include "Pipeline/TextureLoadResult.h"
#include "TextureCookRequestList.h"

#include <dxgiformat.h>

#include <string>
#include <stop_token>

class TextureCookMemoryLimiter;

class TextureAssetCooker final
{
  public:
	bool Cook(
	    const TextureCookRequest& request,
	    TextureCookMemoryLimiter& memoryLimiter,
	    std::stop_token cancellation,
	    std::string& outErrorMessage) const;
};
