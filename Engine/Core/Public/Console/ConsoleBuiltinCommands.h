#pragma once

#include "Core/Public/CoreAPI.h"
#include "Core/Public/Console/CVarControl.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class ConsoleCommandRegistry;
class ConsoleVariableRegistry;
enum class ConsoleCommandScope : std::uint8_t;
struct ConsoleCommandResult;

class SPARKLE_CORE_API ConsoleBuiltinCommands final
{
public:
	static void Register(ConsoleCommandRegistry& commandRegistry, CVarControlExecutor executor);

private:
	static ConsoleCommandResult ExecuteHelp(
	    const ConsoleCommandRegistry& commandRegistry,
	    ConsoleCommandScope scope,
	    std::span<const std::string_view> arguments);
	static ConsoleCommandResult ExecuteListCVars(const CVarControlExecutor& executor, std::span<const std::string_view> arguments);
	static ConsoleCommandResult ExecuteGetCVar(const CVarControlExecutor& executor, std::span<const std::string_view> arguments);
	static ConsoleCommandResult ExecuteSetCVar(const CVarControlExecutor& executor, std::span<const std::string_view> arguments);
	static ConsoleCommandResult FormatControlResult(CVarControlResult result);

	static std::vector<std::string> CompleteCVarName(const ConsoleVariableRegistry& cvarRegistry, std::string_view prefix);
	static std::string FormatCommandHelp(std::string_view name, std::string_view arguments, std::string_view help);
	static std::string FormatCVar(const CVarControlValue& variable);
};
