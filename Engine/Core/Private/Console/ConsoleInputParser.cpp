#include "PCH.h"

#include "Core/Public/Console/ConsoleInputParser.h"
#include "Core/Public/Strings/StringUtils.h"

ConsoleParsedInput ConsoleInputParser::Parse(std::string_view input)
{
	std::vector<std::string> tokens;
	std::string currentToken;
	bool tokenActive = false;
	bool insideQuote = false;
	char quoteCharacter = '\0';

	for (std::size_t index = 0; index < input.size(); ++index)
	{
		const char character = input[index];
		if (character == '\\' && index + 1 < input.size())
		{
			tokenActive = true;
			currentToken.push_back(input[++index]);
			continue;
		}

		if (insideQuote)
		{
			if (character == quoteCharacter)
			{
				tokenActive = true;
				insideQuote = false;
				quoteCharacter = '\0';
			}
			else
			{
				tokenActive = true;
				currentToken.push_back(character);
			}
			continue;
		}

		if (character == '"' || character == '\'')
		{
			tokenActive = true;
			insideQuote = true;
			quoteCharacter = character;
			continue;
		}

		if (Engine::Strings::IsAsciiWhitespace(character))
		{
			if (tokenActive)
			{
				tokens.push_back(std::move(currentToken));
				currentToken.clear();
				tokenActive = false;
			}
			continue;
		}

		tokenActive = true;
		currentToken.push_back(character);
	}

	if (insideQuote)
	{
		return Fail("unterminated quoted string");
	}

	if (tokenActive)
	{
		tokens.push_back(std::move(currentToken));
	}

	ConsoleParsedInput result;
	if (tokens.empty())
	{
		return result;
	}

	result.CommandName = std::move(tokens.front());
	result.Arguments.reserve(tokens.size() - 1);
	for (std::size_t index = 1; index < tokens.size(); ++index)
	{
		result.Arguments.push_back(std::move(tokens[index]));
	}
	return result;
}

ConsoleParsedInput ConsoleInputParser::Fail(std::string errorMessage)
{
	ConsoleParsedInput result;
	result.Succeeded = false;
	result.ErrorMessage = std::move(errorMessage);
	return result;
}
