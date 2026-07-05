#include "PCH.h"

#include "Core/Public/Paths/PathFormatting.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace PathFormatting
{
	std::string TimestampForFileName()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
		std::tm localTime{};
#if defined(_WIN32)
		localtime_s(&localTime, &nowTime);
#else
		localtime_r(&nowTime, &localTime);
#endif

		std::ostringstream stream;
		stream << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
		return stream.str();
	}

	std::filesystem::path TimestampedFileName(std::string_view stem, std::string_view extension)
	{
		std::string fileStem(stem.empty() ? "Sparkle" : stem);
		std::string fileExtension(extension.empty() ? ".log" : extension);
		if (!fileExtension.starts_with('.'))
		{
			fileExtension.insert(fileExtension.begin(), '.');
		}
		return fileStem + "_" + TimestampForFileName() + fileExtension;
	}

	bool IsAsciiAlphaNumeric(char character) noexcept
	{
		const bool isLower = character >= 'a' && character <= 'z';
		const bool isUpper = character >= 'A' && character <= 'Z';
		const bool isDigit = character >= '0' && character <= '9';
		return isLower || isUpper || isDigit;
	}

	char ToLowerAscii(char character) noexcept
	{
		return character >= 'A' && character <= 'Z' ? static_cast<char>(character + 32) : character;
	}

	std::string SanitizePathSegment(std::string_view segment, std::string_view fallback)
	{
		std::string sanitized;
		sanitized.reserve(segment.size());
		for (const char character : segment)
		{
			if (IsAsciiAlphaNumeric(character) || character == '_' || character == '-' || character == '.')
			{
				sanitized.push_back(character);
			}
			else if (character == ' ')
			{
				sanitized.push_back('_');
			}
		}

		return sanitized.empty() ? std::string(fallback) : sanitized;
	}

	bool EndsWithIgnoreCase(std::string_view value, std::string_view suffix) noexcept
	{
		if (suffix.size() > value.size())
		{
			return false;
		}

		const std::string valueTail = std::string(value.substr(value.size() - suffix.size()));
		const std::string suffixString(suffix);
		for (std::size_t index = 0; index < suffix.size(); ++index)
		{
			const char lhs = ToLowerAscii(valueTail[index]);
			const char rhs = ToLowerAscii(suffixString[index]);
			if (lhs != rhs)
			{
				return false;
			}
		}
		return true;
	}
}
