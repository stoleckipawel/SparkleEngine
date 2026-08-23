#include "PCH.h"

#include "Core/Public/Console/ConsoleCommandRegistry.h"

#include "Core/Public/Console/ConsoleInputParser.h"
#include "Core/Public/Strings/StringUtils.h"

ConsoleCommandResult ConsoleCommandResult::Success(std::string message)
{
	return ConsoleCommandResult{.Succeeded = true, .Severity = ConsoleCommandSeverity::Info, .Message = std::move(message)};
}

ConsoleCommandResult ConsoleCommandResult::Warning(std::string message)
{
	return ConsoleCommandResult{.Succeeded = true, .Severity = ConsoleCommandSeverity::Warning, .Message = std::move(message)};
}

ConsoleCommandResult ConsoleCommandResult::Error(std::string message)
{
	return ConsoleCommandResult{.Succeeded = false, .Severity = ConsoleCommandSeverity::Error, .Message = std::move(message)};
}

bool ConsoleCommandRegistry::Register(ConsoleCommandDescriptor descriptor)
{
	if (descriptor.Name.empty())
	{
		return false;
	}
	if (!descriptor.Execute)
	{
		return false;
	}

	const std::string normalizedName = Strings::ToLowerCopy(descriptor.Name);
	if (m_commandIndicesByName.contains(normalizedName))
	{
		return false;
	}

	m_commandIndicesByName.emplace(normalizedName, m_commands.size());
	m_commands.push_back(std::move(descriptor));
	return true;
}

const ConsoleCommandDescriptor* ConsoleCommandRegistry::Find(std::string_view commandName) const
{
	const std::string normalizedName = Strings::ToLowerCopy(commandName);
	const auto iterator = m_commandIndicesByName.find(normalizedName);
	if (iterator == m_commandIndicesByName.end())
	{
		return nullptr;
	}

	return &m_commands[iterator->second];
}

ConsoleCommandResult ConsoleCommandRegistry::ExecuteLine(std::string_view input, ConsoleCommandScope scope) const
{
	const ConsoleParsedInput parsedInput = ConsoleInputParser::Parse(input);
	if (!parsedInput.Succeeded)
	{
		return ConsoleCommandResult::Error(parsedInput.ErrorMessage);
	}
	if (parsedInput.IsEmpty())
	{
		return ConsoleCommandResult::Success();
	}

	const ConsoleCommandDescriptor* descriptor = Find(parsedInput.CommandName);
	if (descriptor == nullptr)
	{
		return ConsoleCommandResult::Error("unknown command: " + parsedInput.CommandName);
	}
	if (!IsScopeAllowed(descriptor->Scope, scope))
	{
		return ConsoleCommandResult::Error("command is not available in this console scope: " + descriptor->Name);
	}

	const std::vector<std::string_view> argumentViews = BuildArgumentViews(parsedInput.Arguments);
	return descriptor->Execute(scope, argumentViews);
}

std::vector<std::string> ConsoleCommandRegistry::CompleteLine(std::string_view input, ConsoleCommandScope scope) const
{
	const bool commandNameOnly = !Strings::ContainsAsciiWhitespace(input);
	if (commandNameOnly)
	{
		std::vector<std::string> completions;
		for (const ConsoleCommandDescriptor& command : m_commands)
		{
			if (IsScopeAllowed(command.Scope, scope) && Strings::StartsWithIgnoreCase(command.Name, input))
			{
				completions.push_back(command.Name);
			}
		}
		return completions;
	}

	const ConsoleParsedInput parsedInput = ConsoleInputParser::Parse(input);
	if (!parsedInput.Succeeded || parsedInput.CommandName.empty())
	{
		return {};
	}

	const ConsoleCommandDescriptor* descriptor = Find(parsedInput.CommandName);
	if (descriptor == nullptr || !descriptor->Complete || !IsScopeAllowed(descriptor->Scope, scope))
	{
		return {};
	}

	const std::vector<std::string_view> argumentViews = BuildArgumentViews(parsedInput.Arguments);
	const std::string_view currentToken = argumentViews.empty() ? std::string_view{} : argumentViews.back();
	return descriptor->Complete(
	    scope,
	    ConsoleAutocompleteRequest{
	        .CommandName = descriptor->Name,
	        .Arguments = argumentViews,
	        .CurrentToken = currentToken,
	    });
}

bool ConsoleCommandRegistry::IsScopeAllowed(ConsoleCommandScope commandScope, ConsoleCommandScope contextScope) noexcept
{
	if (commandScope == ConsoleCommandScope::Runtime)
	{
		return true;
	}
	if (commandScope == ConsoleCommandScope::Editor)
	{
		return contextScope == ConsoleCommandScope::Editor || contextScope == ConsoleCommandScope::Developer;
	}
	return contextScope == ConsoleCommandScope::Developer;
}

std::vector<std::string_view> ConsoleCommandRegistry::BuildArgumentViews(const std::vector<std::string>& arguments)
{
	std::vector<std::string_view> argumentViews;
	argumentViews.reserve(arguments.size());
	for (const std::string& argument : arguments)
	{
		argumentViews.push_back(argument);
	}
	return argumentViews;
}
