#pragma once

#include "TextureCookRequestList.h"

#include <string>

class DefaultTextureCookRequestBuilder final
{
  public:
	DefaultTextureCookRequestBuilder() = delete;

	static bool AppendTo(TextureCookRequestSet& requestSet, std::string& outErrorMessage);
};
