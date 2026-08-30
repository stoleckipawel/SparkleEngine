#pragma once

#include "TextureCookRequestList.h"

class DefaultTextureCookRequestBuilder final
{
public:
	DefaultTextureCookRequestBuilder() = delete;

	static void AppendTo(TextureCookRequestSet& requestSet);
};
