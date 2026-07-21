#pragma once

#include "TextureCookRequestList.h"

#include <string>

namespace DefaultTextureCookRequestBuilder
{
	bool AppendTo(TextureCookRequestSet& requestSet, std::string& outErrorMessage);
}
