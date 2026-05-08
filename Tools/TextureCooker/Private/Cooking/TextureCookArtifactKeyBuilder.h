#pragma once

#include "CookArtifactCache.h"
#include "TextureCookRequestList.h"

#include <string>

namespace AssetAuthoring
{
	class TextureCookArtifactKeyBuilder final
	{
	  public:
		static bool TryBuild(
			const TextureCookRequest& request,
			Cook::CookArtifactKey& outKey,
			std::string& outErrorMessage);
	};
}