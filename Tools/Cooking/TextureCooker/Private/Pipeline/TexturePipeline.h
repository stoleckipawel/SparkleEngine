#pragma once

#include "Pipeline/TextureLoadResult.h"
#include "TextureCookRequestList.h"

class TexturePipeline final
{
public:
	static TextureLoadResult Process(const TextureCookRequest& request, TextureLoadResult sourceTexture);
};
