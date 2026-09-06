#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class ConsoleCommandScope : std::uint8_t
{
	Runtime = 0,
	Editor,
	Developer,
};

enum class ConsoleCommandSeverity : std::uint8_t
{
	Info = 0,
	Warning,
	Error,
};

struct SPARKLE_CORE_API ConsoleCommandResult final
{
	bool Succeeded = true;
	ConsoleCommandSeverity Severity = ConsoleCommandSeverity::Info;
	std::string Message;

	static ConsoleCommandResult Success(std::string message = {});
	static ConsoleCommandResult Warning(std::string message);
	static ConsoleCommandResult Error(std::string message);
};

struct SPARKLE_CORE_API ConsoleAutocompleteRequest final
{
	std::string_view CommandName;
	std::span<const std::string_view> Arguments;
	std::string_view CurrentToken;
};

using ConsoleCommandCallback = std::function<ConsoleCommandResult(ConsoleCommandScope, std::span<const std::string_view>)>;
using ConsoleAutocompleteCallback = std::function<std::vector<std::string>(ConsoleCommandScope, const ConsoleAutocompleteRequest&)>;

struct SPARKLE_CORE_API ConsoleCommandDescriptor final
{
	std::string Name;
	std::string Help;
	std::string ArgumentSyntax;
	ConsoleCommandScope Scope = ConsoleCommandScope::Runtime;
	ConsoleCommandCallback Execute;
	ConsoleAutocompleteCallback Complete;
};

class SPARKLE_CORE_API ConsoleCommandRegistry final
{
public:
	bool Register(ConsoleCommandDescriptor descriptor);

	const ConsoleCommandDescriptor* Find(std::string_view commandName) const;
	const std::vector<ConsoleCommandDescriptor>& GetCommands() const noexcept { return m_commands; }

	ConsoleCommandResult ExecuteLine(std::string_view input, ConsoleCommandScope scope) const;
	std::vector<std::string> CompleteLine(std::string_view input, ConsoleCommandScope scope) const;

private:
	static bool IsScopeAllowed(ConsoleCommandScope commandScope, ConsoleCommandScope contextScope) noexcept;
	static std::vector<std::string_view> BuildArgumentViews(const std::vector<std::string>& arguments);

	std::vector<ConsoleCommandDescriptor> m_commands;
	std::unordered_map<std::string, std::size_t> m_commandIndicesByName;
};
