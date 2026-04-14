#pragma once

#include <DirectXMath.h>

#include <filesystem>
#include <sstream>
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

		inline std::string TrimCopy(std::string_view str)
		{
			return std::string(TrimAsciiWhitespace(str));
		}

		inline std::string UnquoteCopy(std::string_view str)
		{
			return std::string(Unquote(TrimAsciiWhitespace(str)));
		}

		std::string ToLowerCopy(std::string_view str);

		bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept;
		bool ContainsIgnoreCase(std::string_view haystack, std::string_view needle) noexcept;
		bool TryParseBool(std::string_view str, bool& outValue);
		bool TrySplitKeyValue(std::string_view str, char separator, std::string_view& outKey, std::string_view& outValue) noexcept;

		inline bool TryParseFloat(std::string_view str, float& outValue)
		{
			const std::string trimmed = TrimCopy(str);
			if (trimmed.empty())
			{
				return false;
			}

			try
			{
				std::size_t parsedLength = 0;
				outValue = std::stof(trimmed, &parsedLength);
				return parsedLength == trimmed.size();
			}
			catch (...)
			{
				return false;
			}
		}

		inline bool TryParseFloat3(std::string_view str, DirectX::XMFLOAT3& outValue)
		{
			std::stringstream stream{std::string(str)};
			std::string segment;
			float values[3] = {};

			for (int index = 0; index < 3; ++index)
			{
				if (!std::getline(stream, segment, ','))
				{
					return false;
				}

				if (!TryParseFloat(segment, values[index]))
				{
					return false;
				}
			}

			outValue = {values[0], values[1], values[2]};
			return true;
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
	}  // namespace Strings
}  // namespace Engine
