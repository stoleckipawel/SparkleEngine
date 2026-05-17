#pragma once

#include "Pipeline/TextureLoadResult.h"
#include "TextureCookRequestList.h"

#include <dxgiformat.h>

#include <string>

	class TextureAssetCooker final
	{
	  public:
		bool Cook(const TextureCookRequest& request, std::string& outErrorMessage) const;
	};