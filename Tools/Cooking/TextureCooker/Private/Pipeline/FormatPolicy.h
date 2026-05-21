#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

namespace TextureCookPipeline
{
	DXGI_FORMAT ResolveUncompressedOutputFormat(const TextureCookRequest& request, bool sourceWasFloat) noexcept;
	TextureFormatIntent ResolveFormatIntent(DXGI_FORMAT format) noexcept;
	DXGI_FORMAT ApplyRequestedColorSpace(DXGI_FORMAT format, TextureColorSpace colorSpace) noexcept;

	bool IsCompressedFormat(DXGI_FORMAT format) noexcept;
	bool IsFloatFormat(DXGI_FORMAT format) noexcept;
	bool IsByteRgbaFormat(DXGI_FORMAT format) noexcept;
	bool IsSrgbCapableFormat(DXGI_FORMAT format) noexcept;
	bool HasMeaningfulAlpha(const WorkingTexture& workingTexture) noexcept;
	bool IsGreyscaleLike(const WorkingTexture& workingTexture) noexcept;
}