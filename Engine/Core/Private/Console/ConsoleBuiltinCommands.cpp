#include "PCH.h"

#include "Core/Public/Console/ConsoleBuiltinCommands.h"

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/CVar.h"
#include "Core/Public/Console/CVarRegistry.h"
#include "Core/Public/Strings/StringUtils.h"

void ConsoleBuiltinCommands::Register(ConsoleCommandRegistry& commandRegistry, ConsoleVariableRegistry& cvarRegistry)
{
	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "Help",
	    .Help = "Lists console commands or filters command help.",
	    .ArgumentSyntax = "[filter]",
	    .Scope = ConsoleCommandScope::Runtime,
	    .Execute = [&commandRegistry](const ConsoleCommandContext& context, std::span<const std::string_view> arguments) {
		    return ExecuteHelp(commandRegistry, context, arguments);
	    },
	});

	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "ListCVars",
	    .Help = "Lists registered console variables.",
	    .ArgumentSyntax = "[filter]",
	    .Scope = ConsoleCommandScope::Runtime,
	    .Execute = [&cvarRegistry](const ConsoleCommandContext&, std::span<const std::string_view> arguments) {
		    return ExecuteListCVars(cvarRegistry, arguments);
	    },
	    .Complete = [&cvarRegistry](const ConsoleCommandContext&, const ConsoleAutocompleteRequest& request) {
		    return CompleteCVarName(cvarRegistry, request.CurrentToken);
	    },
	});

	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "GetCVar",
	    .Help = "Prints a console variable value.",
	    .ArgumentSyntax = "<name>",
	    .Scope = ConsoleCommandScope::Runtime,
	    .Execute = [&cvarRegistry](const ConsoleCommandContext&, std::span<const std::string_view> arguments) {
		    return ExecuteGetCVar(cvarRegistry, arguments);
	    },
	    .Complete = [&cvarRegistry](const ConsoleCommandContext&, const ConsoleAutocompleteRequest& request) {
		    return CompleteCVarName(cvarRegistry, request.CurrentToken);
	    },
	});

	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "SetCVar",
	    .Help = "Sets a console variable value.",
	    .ArgumentSyntax = "<name> <value>",
	    .Scope = ConsoleCommandScope::Runtime,
	    .Execute = [&cvarRegistry](const ConsoleCommandContext&, std::span<const std::string_view> arguments) {
		    return ExecuteSetCVar(cvarRegistry, arguments);
	    },
	    .Complete = [&cvarRegistry](const ConsoleCommandContext&, const ConsoleAutocompleteRequest& request) {
		    return CompleteCVarName(cvarRegistry, request.CurrentToken);
	    },
	});
}

void ConsoleBuiltinCommands::Register(ConsoleCommandRegistry& commandRegistry)
{
	Register(commandRegistry, ConsoleVariableRegistry::Get());
}

ConsoleCommandResult ConsoleBuiltinCommands::ExecuteHelp(
	const ConsoleCommandRegistry& commandRegistry,
	const ConsoleCommandContext& context,
	std::span<const std::string_view> arguments)
{
	const std::string_view filter = arguments.empty() ? std::string_view{} : arguments.front();
	std::string output;
	for (const ConsoleCommandDescriptor& command : commandRegistry.GetCommands())
	{
		const bool scopeAllowed = command.Scope == ConsoleCommandScope::Runtime ||
		    context.Scope == ConsoleCommandScope::Developer ||
		    (command.Scope == ConsoleCommandScope::Editor && context.Scope == ConsoleCommandScope::Editor);
		if (!scopeAllowed)
		{
			continue;
		}
		if (!filter.empty() && !Engine::Strings::ContainsIgnoreCase(command.Name, filter) && !Engine::Strings::ContainsIgnoreCase(command.Help, filter))
		{
			continue;
		}

		if (!output.empty())
		{
			output += '\n';
		}
		output += FormatCommandHelp(command.Name, command.ArgumentSyntax, command.Help);
	}

	if (output.empty())
	{
		return ConsoleCommandResult::Warning("no commands matched");
	}
	return ConsoleCommandResult::Success(output);
}

ConsoleCommandResult ConsoleBuiltinCommands::ExecuteListCVars(
	const ConsoleVariableRegistry& cvarRegistry,
	std::span<const std::string_view> arguments)
{
	const std::string_view filter = arguments.empty() ? std::string_view{} : arguments.front();
	std::string output;
	for (const ConsoleVariableBase* variable : cvarRegistry.GetVariables())
	{
		if (variable == nullptr)
		{
			continue;
		}
		if (!filter.empty() && !Engine::Strings::ContainsIgnoreCase(variable->GetName(), filter) && !Engine::Strings::ContainsIgnoreCase(variable->GetDescription(), filter))
		{
			continue;
		}

		if (!output.empty())
		{
			output += '\n';
		}
		output += FormatCVar(*variable);
	}

	if (output.empty())
	{
		return ConsoleCommandResult::Warning("no CVars matched");
	}
	return ConsoleCommandResult::Success(output);
}

ConsoleCommandResult ConsoleBuiltinCommands::ExecuteGetCVar(
	const ConsoleVariableRegistry& cvarRegistry,
	std::span<const std::string_view> arguments)
{
	if (arguments.size() != 1)
	{
		return ConsoleCommandResult::Error("usage: GetCVar <name>");
	}

	const ConsoleVariableBase* variable = cvarRegistry.Find(arguments.front());
	if (variable == nullptr)
	{
		return ConsoleCommandResult::Error("unknown CVar: " + std::string(arguments.front()));
	}

	return ConsoleCommandResult::Success(FormatCVar(*variable));
}

ConsoleCommandResult ConsoleBuiltinCommands::ExecuteSetCVar(
	ConsoleVariableRegistry& cvarRegistry,
	std::span<const std::string_view> arguments)
{
	if (arguments.size() < 2)
	{
		return ConsoleCommandResult::Error("usage: SetCVar <name> <value>");
	}

	ConsoleVariableBase* variable = cvarRegistry.Find(arguments.front());
	if (variable == nullptr)
	{
		return ConsoleCommandResult::Error("unknown CVar: " + std::string(arguments.front()));
	}

	const std::string value = Engine::Strings::Join(arguments, " ", 1);
	std::string errorMessage;
	if (!variable->TrySetValueFromString(value, errorMessage))
	{
		return ConsoleCommandResult::Error("failed to set " + std::string(variable->GetName()) + ": " + errorMessage);
	}

	return ConsoleCommandResult::Success(FormatCVar(*variable));
}

std::vector<std::string> ConsoleBuiltinCommands::CompleteCVarName(const ConsoleVariableRegistry& cvarRegistry, std::string_view prefix)
{
	std::vector<std::string> completions;
	for (const ConsoleVariableBase* variable : cvarRegistry.GetVariables())
	{
		if (variable != nullptr && Engine::Strings::StartsWithIgnoreCase(variable->GetName(), prefix))
		{
			completions.emplace_back(variable->GetName());
		}
	}
	return completions;
}

std::string ConsoleBuiltinCommands::FormatCommandHelp(std::string_view name, std::string_view arguments, std::string_view help)
{
	std::string output(name);
	if (!arguments.empty())
	{
		output += ' ';
		output += arguments;
	}
	if (!help.empty())
	{
		output += " - ";
		output += help;
	}
	return output;
}

std::string ConsoleBuiltinCommands::FormatCVar(const ConsoleVariableBase& variable)
{
	std::string output(variable.GetName());
	output += " = ";
	output += variable.GetValueAsString();
	output += " (";
	output += variable.GetValueTypeName();
	output += ')';
	if (!variable.GetDescription().empty())
	{
		output += " - ";
		output += variable.GetDescription();
	}
	return output;
}
