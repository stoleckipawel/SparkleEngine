#include "PCH.h"

#include "Core/Public/Process/CommandLineUtils.h"

#include <cwctype>

namespace CommandLine
{
	std::wstring_view ReadToken(std::wstring_view commandLine, std::size_t& offset) noexcept
	{
		while (offset < commandLine.size() && std::iswspace(commandLine[offset]))
		{
			++offset;
		}

		if (offset >= commandLine.size())
		{
			return {};
		}

		const std::size_t tokenStart = offset;
		if (commandLine[offset] == L'"')
		{
			++offset;
			const std::size_t quotedStart = offset;
			while (offset < commandLine.size() && commandLine[offset] != L'"')
			{
				++offset;
			}
			const std::size_t quotedEnd = offset;
			if (offset < commandLine.size())
			{
				++offset;
			}
			return commandLine.substr(quotedStart, quotedEnd - quotedStart);
		}

		while (offset < commandLine.size() && !std::iswspace(commandLine[offset]))
		{
			++offset;
		}
		return commandLine.substr(tokenStart, offset - tokenStart);
	}

	std::string QuoteArgument(std::string_view text)
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

	std::string QuotePath(const std::filesystem::path& path)
	{
		return QuoteArgument(path.string());
	}
}
