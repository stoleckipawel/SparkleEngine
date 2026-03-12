#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Engine
{
	namespace Strings
	{
		constexpr std::string_view TrimAsciiWhitespace(std::string_view str) noexcept
		{
			constexpr std::string_view kWhitespace = " \t\r\n";
			const auto start = str.find_first_not_of(kWhitespace);
			if (start == std::string_view::npos)
			{
				return {};
			}
			const auto end = str.find_last_not_of(kWhitespace);
			return str.substr(start, end - start + 1);
		}

		constexpr std::string_view Unquote(std::string_view str) noexcept
		{
			if (str.size() >= 2 && str.front() == '"' && str.back() == '"')
			{
				return str.substr(1, str.size() - 2);
			}
			return str;
		}

		inline std::wstring ToWide(std::string_view str)
		{
			return std::wstring(str.begin(), str.end());
		}

		inline std::wstring ToWide(const std::filesystem::path& path)
		{
			return path.wstring();
		}

		inline std::string ToNarrow(std::wstring_view str)
		{
			return std::string(str.begin(), str.end());
		}
	}
}
