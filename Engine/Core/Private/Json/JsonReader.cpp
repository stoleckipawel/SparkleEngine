#include "PCH.h"

#include "Core/Public/Json/JsonReader.h"

#include "Core/Public/Formatting/HexFormat.h"

#include <cctype>
#include <utility>

namespace Json
{
	std::size_t FindPropertyValue(std::string_view objectText, std::string_view key) noexcept
	{
		const std::string needle = "\"" + std::string(key) + "\"";
		const std::size_t keyOffset = objectText.find(needle);
		if (keyOffset == std::string_view::npos)
		{
			return std::string_view::npos;
		}

		std::size_t cursor = objectText.find(':', keyOffset + needle.size());
		if (cursor == std::string_view::npos)
		{
			return std::string_view::npos;
		}

		++cursor;
		while (cursor < objectText.size() && std::isspace(static_cast<unsigned char>(objectText[cursor])))
		{
			++cursor;
		}
		return cursor;
	}

	bool TryReadStringProperty(std::string_view objectText, std::string_view key, std::string& outValue)
	{
		std::size_t cursor = FindPropertyValue(objectText, key);
		if (cursor == std::string_view::npos || cursor >= objectText.size() || objectText[cursor] != '"')
		{
			return false;
		}
		++cursor;

		std::string value;
		for (; cursor < objectText.size(); ++cursor)
		{
			const char character = objectText[cursor];
			if (character == '"')
			{
				outValue = std::move(value);
				return true;
			}
			if (character != '\\')
			{
				value.push_back(character);
				continue;
			}

			++cursor;
			if (cursor >= objectText.size())
			{
				return false;
			}

			switch (objectText[cursor])
			{
				case '"':
					value.push_back('"');
					break;
				case '\\':
					value.push_back('\\');
					break;
				case '/':
					value.push_back('/');
					break;
				case 'b':
					value.push_back('\b');
					break;
				case 'f':
					value.push_back('\f');
					break;
				case 'n':
					value.push_back('\n');
					break;
				case 'r':
					value.push_back('\r');
					break;
				case 't':
					value.push_back('\t');
					break;
				default:
					return false;
			}
		}

		return false;
	}

	bool TryReadUInt64Property(std::string_view objectText, std::string_view key, std::uint64_t& outValue)
	{
		return TryReadUnsignedIntegerProperty(objectText, key, outValue);
	}

	bool TryParseHexUInt64(std::string_view text, std::uint64_t& outValue) noexcept
	{
		return Formatting::TryParseHexUInt64(text, outValue);
	}
}
