#pragma once

#include "Pipeline/TextureLoadResult.h"
#include "TextureCookRequestList.h"

#include <dxgiformat.h>

#include <cstddef>

class TextureCookMemoryLimiter;

class TextureAssetCooker final
{
public:
	void Cook(const TextureCookRequest& request, TextureCookMemoryLimiter& memoryLimiter) const;

private:
	static std::size_t CalculatePixelDataBytes(const TextureLoadResult& texture);
};
