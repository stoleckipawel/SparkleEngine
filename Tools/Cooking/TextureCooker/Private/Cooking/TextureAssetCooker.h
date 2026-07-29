#pragma once

#include "Pipeline/TextureLoadResult.h"
#include "TextureCookRequestList.h"

#include <dxgiformat.h>

#include <stop_token>

class TextureCookMemoryLimiter;

class TextureAssetCooker final
{
  public:
	void Cook(
	    const TextureCookRequest& request,
	    TextureCookMemoryLimiter& memoryLimiter,
	    std::stop_token cancellation) const;
};
