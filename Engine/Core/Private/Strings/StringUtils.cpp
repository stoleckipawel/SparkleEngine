#include "PCH.h"

#include "Core/Public/Strings/StringUtils.h"

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Strings
{
	bool IsAsciiWhitespace(char character) noexcept
	{
		return character == ' ' || character == '\t' || character == '\r' || character == '\n';
	}

	bool ContainsAsciiWhitespace(std::string_view str) noexcept
	{
		return std::ranges::any_of(str, IsAsciiWhitespace);
	}

	bool IsNullTerminated(std::span<const char> buffer) noexcept
	{
		return std::ranges::any_of(buffer, [](const char character) { return character == '\0'; });
	}

	std::string_view TrimAsciiWhitespace(std::string_view str) noexcept
	{
		constexpr std::string_view whitespace = " \t\r\n";
		const std::size_t start = str.find_first_not_of(whitespace);
		if (start == std::string_view::npos)
		{
			return {};
		}
		const std::size_t end = str.find_last_not_of(whitespace);
		return str.substr(start, end - start + 1);
	}

	std::string_view Unquote(std::string_view str) noexcept
	{
		constexpr std::size_t quoteCharacterCount = 2;
		if (str.size() >= quoteCharacterCount && str.front() == '"' && str.back() == '"')
		{
			return str.substr(1, str.size() - quoteCharacterCount);
		}
		return str;
	}

	std::string TrimCopy(std::string_view str)
	{
		return std::string(TrimAsciiWhitespace(str));
	}

	std::string UnquoteCopy(std::string_view str)
	{
		return std::string(Unquote(TrimAsciiWhitespace(str)));
	}

	std::vector<std::string_view> Split(std::string_view str, char separator, bool keepEmptyFields)
	{
		std::vector<std::string_view> fields;
		std::size_t start = 0;
		while (start <= str.size())
		{
			const std::size_t separatorIndex = str.find(separator, start);
			const std::size_t end = separatorIndex == std::string_view::npos ? str.size() : separatorIndex;
			const std::string_view field = str.substr(start, end - start);
			if (keepEmptyFields || !field.empty())
			{
				fields.push_back(field);
			}

			if (separatorIndex == std::string_view::npos)
			{
				break;
			}
			start = separatorIndex + 1;
		}
		return fields;
	}

	std::string ToLowerCopy(std::string_view str)
	{
		std::string lowered(str);
		std::ranges::transform(
		    lowered,
		    lowered.begin(),
		    [](const unsigned char character) { return static_cast<char>(std::tolower(character)); });
		return lowered;
	}

	std::string EscapeCsvField(std::string_view str)
	{
		constexpr std::size_t surroundingQuoteCount = 2;
		std::string result;
		result.reserve(str.size() + surroundingQuoteCount);
		result.push_back('"');
		for (const char character : str)
		{
			if (character == '"')
			{
				result.push_back('"');
			}
			result.push_back(character);
		}
		result.push_back('"');
		return result;
	}

	std::string EscapeJsonString(std::string_view str)
	{
		constexpr std::size_t escapeCapacitySlack = 8;
		std::string result;
		result.reserve(str.size() + escapeCapacitySlack);
		for (const char character : str)
		{
			switch (character)
			{
				case '\\':
					result += "\\\\";
					break;
				case '"':
					result += "\\\"";
					break;
				case '\n':
					result += "\\n";
					break;
				case '\r':
					result += "\\r";
					break;
				case '\t':
					result += "\\t";
					break;
				default:
					result.push_back(character);
					break;
			}
		}
		return result;
	}

	bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept
	{
		if (lhs.size() != rhs.size())
		{
			return false;
		}

		for (std::size_t index = 0; index < lhs.size(); ++index)
		{
			const unsigned char lhsChar = static_cast<unsigned char>(lhs[index]);
			const unsigned char rhsChar = static_cast<unsigned char>(rhs[index]);
			if (std::tolower(lhsChar) != std::tolower(rhsChar))
			{
				return false;
			}
		}

		return true;
	}

	bool StartsWithIgnoreCase(std::string_view value, std::string_view prefix) noexcept
	{
		if (prefix.size() > value.size())
		{
			return false;
		}

		for (std::size_t index = 0; index < prefix.size(); ++index)
		{
			const unsigned char valueCharacter = static_cast<unsigned char>(value[index]);
			const unsigned char prefixCharacter = static_cast<unsigned char>(prefix[index]);
			if (std::tolower(valueCharacter) != std::tolower(prefixCharacter))
			{
				return false;
			}
		}
		return true;
	}

	bool ContainsIgnoreCase(std::string_view haystack, std::string_view needle) noexcept
	{
		if (needle.empty())
		{
			return true;
		}

		if (needle.size() > haystack.size())
		{
			return false;
		}

		const std::size_t finalStartIndex = haystack.size() - needle.size();
		for (std::size_t startIndex = 0; startIndex <= finalStartIndex; ++startIndex)
		{
			bool matches = true;
			for (std::size_t needleIndex = 0; needleIndex < needle.size(); ++needleIndex)
			{
				const unsigned char haystackCharacter = static_cast<unsigned char>(haystack[startIndex + needleIndex]);
				const unsigned char needleCharacter = static_cast<unsigned char>(needle[needleIndex]);
				if (std::tolower(haystackCharacter) != std::tolower(needleCharacter))
				{
					matches = false;
					break;
				}
			}

			if (matches)
			{
				return true;
			}
		}

		return false;
	}

	bool TryParseBool(std::string_view str, bool& outValue)
	{
		const std::string normalized = ToLowerCopy(TrimAsciiWhitespace(str));
		if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on")
		{
			outValue = true;
			return true;
		}

		if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off")
		{
			outValue = false;
			return true;
		}

		return false;
	}

	bool TrySplitKeyValue(std::string_view str, char separator, std::string_view& outKey, std::string_view& outValue) noexcept
	{
		const std::size_t separatorIndex = str.find(separator);
		if (separatorIndex == std::string::npos)
		{
			return false;
		}

		outKey = TrimAsciiWhitespace(str.substr(0, separatorIndex));
		outValue = TrimAsciiWhitespace(str.substr(separatorIndex + 1));
		return true;
	}

	std::string Join(std::span<const std::string_view> values, std::string_view separator, std::size_t firstValueIndex)
	{
		std::string output;
		for (std::size_t index = firstValueIndex; index < values.size(); ++index)
		{
			if (index > firstValueIndex)
			{
				output += separator;
			}
			output += values[index];
		}
		return output;
	}

	bool TryParseFloat(std::string_view str, float& outValue)
	{
		const std::string trimmed = TrimCopy(str);
		if (trimmed.empty())
		{
			return false;
		}

		try
		{
			std::size_t parsedLength = 0;
			const float parsedValue = std::stof(trimmed, &parsedLength);
			if (parsedLength != trimmed.size())
			{
				return false;
			}
			outValue = parsedValue;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	bool TryParseFloat3(std::string_view str, DirectX::XMFLOAT3& outValue)
	{
		constexpr std::size_t componentCount = 3;
		std::stringstream stream{std::string(str)};
		std::string segment;
		std::array<float, componentCount> values{};
		for (float& value : values)
		{
			if (!std::getline(stream, segment, ',') || !TryParseFloat(segment, value))
			{
				return false;
			}
		}
		if (std::getline(stream, segment, ','))
		{
			return false;
		}

		outValue = {values.front(), values[1], values.back()};
		return true;
	}

	std::wstring ToWide(std::string_view str)
	{
		return {str.begin(), str.end()};
	}

	std::wstring ToWide(const std::filesystem::path& path)
	{
		return path.wstring();
	}

	std::string ToNarrow(std::wstring_view str)
	{
		std::string result;
		result.reserve(str.size());
		for (const wchar_t character : str)
		{
			result.push_back(static_cast<char>(character));
		}
		return result;
	}
}
