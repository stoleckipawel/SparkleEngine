#pragma once

#include "Core/Public/CoreAPI.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

class ConsoleCommandRegistry;
class ConsoleVariableBase;
class ConsoleVariableRegistry;
struct ConsoleCommandContext;
struct ConsoleCommandResult;

class SPARKLE_CORE_API ConsoleBuiltinCommands final
{
  public:
	static void Register(ConsoleCommandRegistry& commandRegistry, ConsoleVariableRegistry& cvarRegistry);
	static void Register(ConsoleCommandRegistry& commandRegistry);

  private:
	static ConsoleCommandResult ExecuteHelp(
	    const ConsoleCommandRegistry& commandRegistry,
	    const ConsoleCommandContext& context,
	    std::span<const std::string_view> arguments);
	static ConsoleCommandResult ExecuteListCVars(
	    const ConsoleVariableRegistry& cvarRegistry,
	    std::span<const std::string_view> arguments);
	static ConsoleCommandResult ExecuteGetCVar(
	    const ConsoleVariableRegistry& cvarRegistry,
	    std::span<const std::string_view> arguments);
	static ConsoleCommandResult ExecuteSetCVar(
	    ConsoleVariableRegistry& cvarRegistry,
	    std::span<const std::string_view> arguments);

	static std::vector<std::string> CompleteCVarName(const ConsoleVariableRegistry& cvarRegistry, std::string_view prefix);
	static std::string FormatCommandHelp(std::string_view name, std::string_view arguments, std::string_view help);
	static std::string FormatCVar(const ConsoleVariableBase& variable);
};
