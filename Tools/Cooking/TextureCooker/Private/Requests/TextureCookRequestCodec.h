#pragma once

#include "TextureCookRequestList.h"

#include <string>
#include <string_view>

namespace TextureCookRequestCodec
{
	std::string_view GetHeader() noexcept;
	bool IsHeader(std::string_view line) noexcept;
	TextureCookRequest ParseLine(std::string_view line);
	std::string FormatLine(const TextureCookRequest& request);
}
