#pragma once

#include "Pipeline/TextureLoadResult.h"
#include "TextureCookRequestList.h"

#include <string>

	class TexturePipeline final
	{
	  public:
		static bool Process(
		    const TextureCookRequest& request,
		    TextureLoadResult sourceTexture,
		    TextureLoadResult& outProcessedTexture,
		    std::string& outErrorMessage);
	};
