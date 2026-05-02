#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace CommandLine
{
	inline std::string QuoteArgument(std::string_view text)
	{
		std::string quoted;
		quoted.reserve(text.size() + 2);
		quoted.push_back('"');
		for (const char character : text)
		{
			if (character == '"')
			{
				quoted.push_back('\\');
			}
			quoted.push_back(character);
		}
		quoted.push_back('"');
		return quoted;
	}

	inline std::string QuotePath(const std::filesystem::path& path)
	{
		return QuoteArgument(path.string());
	}
}  // namespace CommandLine