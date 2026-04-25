#include "Core/Public/Console/ConsoleBuiltinCommands.h"
#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleInputParser.h"
#include "Core/Public/Console/ConsoleSession.h"
#include "Core/Public/Console/CVar.h"
#include "Core/Public/Console/CVarRegistry.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

enum class ConsoleDiagnosticMode : std::uint32_t
{
	First = 1,
	Second = 2,
};

class ConsoleDiagnostics final
{
  public:
	int Run()
	{
		ConsoleVariable<bool> boolCVar("test.Bool", false, "Test bool CVar.");
		ConsoleVariable<int> intCVar("test.Int", 3, "Test integer CVar.");
		ConsoleVariable<float> floatCVar("test.Float", 1.0f, "Test float CVar.");
		ConsoleVariable<std::string> stringCVar("test.String", std::string("initial"), "Test string CVar.");
		ConsoleVariable<ConsoleDiagnosticMode> enumCVar("test.Enum", ConsoleDiagnosticMode::First, "Test enum CVar.");

		ConsoleCommandRegistry commandRegistry;
		ConsoleBuiltinCommands::Register(commandRegistry, ConsoleVariableRegistry::Get());

		ExpectParserHandlesQuotedArguments();
		ExpectRuntimeCVarCommands(commandRegistry, boolCVar, intCVar, floatCVar, stringCVar, enumCVar);
		ExpectAutocomplete(commandRegistry);
		ExpectCommandScopeFiltering(commandRegistry);
		ExpectConsoleSession(commandRegistry, intCVar);

		if (m_failureCount != 0)
		{
			std::cerr << "ConsoleDiagnostics failed with " << m_failureCount << " failure(s).\n";
			return 1;
		}

		std::cout << "ConsoleDiagnostics passed.\n";
		return 0;
	}

  private:
	void ExpectParserHandlesQuotedArguments()
	{
		const ConsoleParsedInput parsedInput = ConsoleInputParser::Parse("SetCVar test.String \"hello world\"");
		Expect(parsedInput.Succeeded, "parser should accept quoted arguments");
		Expect(parsedInput.CommandName == "SetCVar", "parser command name should match");
		Expect(parsedInput.Arguments.size() == 2, "parser should return two arguments");
		Expect(parsedInput.Arguments.size() > 1 && parsedInput.Arguments[1] == "hello world", "parser should preserve quoted whitespace");

		const ConsoleParsedInput badInput = ConsoleInputParser::Parse("SetCVar test.String \"unterminated");
		Expect(!badInput.Succeeded, "parser should reject unterminated quotes");
	}

	void ExpectRuntimeCVarCommands(
		ConsoleCommandRegistry& commandRegistry,
		ConsoleVariable<bool>& boolCVar,
		ConsoleVariable<int>& intCVar,
		ConsoleVariable<float>& floatCVar,
		ConsoleVariable<std::string>& stringCVar,
		ConsoleVariable<ConsoleDiagnosticMode>& enumCVar)
	{
		const ConsoleCommandContext runtimeContext{.Scope = ConsoleCommandScope::Runtime};
		Expect(commandRegistry.ExecuteLine("Help", runtimeContext).Succeeded, "Help should execute");
		Expect(commandRegistry.ExecuteLine("ListCVars test.", runtimeContext).Succeeded, "ListCVars should execute");

		Expect(commandRegistry.ExecuteLine("SetCVar test.Bool on", runtimeContext).Succeeded, "SetCVar should parse bool aliases");
		Expect(boolCVar.Get(), "bool CVar should be updated");

		Expect(commandRegistry.ExecuteLine("SetCVar test.Int 42", runtimeContext).Succeeded, "SetCVar should parse integers");
		Expect(intCVar.Get() == 42, "integer CVar should be updated");

		Expect(commandRegistry.ExecuteLine("SetCVar test.Float 2.5", runtimeContext).Succeeded, "SetCVar should parse floats");
		Expect(floatCVar.Get() > 2.49f && floatCVar.Get() < 2.51f, "float CVar should be updated");

		Expect(commandRegistry.ExecuteLine("SetCVar test.String \"hello runtime\"", runtimeContext).Succeeded, "SetCVar should parse strings");
		Expect(stringCVar.Get() == "hello runtime", "string CVar should be updated");
		Expect(commandRegistry.ExecuteLine("SetCVar test.String \"\"", runtimeContext).Succeeded, "SetCVar should preserve empty quoted strings");
		Expect(stringCVar.Get().empty(), "string CVar should accept empty quoted strings");

		Expect(commandRegistry.ExecuteLine("SetCVar test.Enum 2", runtimeContext).Succeeded, "SetCVar should parse enum underlying values");
		Expect(enumCVar.Get() == ConsoleDiagnosticMode::Second, "enum CVar should be updated");

		const ConsoleCommandResult getResult = commandRegistry.ExecuteLine("GetCVar test.Bool", runtimeContext);
		Expect(getResult.Succeeded && getResult.Message.find("true") != std::string::npos, "GetCVar should report updated bool value");

		Expect(!commandRegistry.ExecuteLine("SetCVar test.Bool maybe", runtimeContext).Succeeded, "SetCVar should reject invalid values");
		Expect(!commandRegistry.ExecuteLine("MissingCommand", runtimeContext).Succeeded, "unknown commands should fail");
	}

	void ExpectAutocomplete(ConsoleCommandRegistry& commandRegistry)
	{
		const ConsoleCommandContext runtimeContext{.Scope = ConsoleCommandScope::Runtime};
		const std::vector<std::string> commandCompletions = commandRegistry.CompleteLine("Set", runtimeContext);
		Expect(Contains(commandCompletions, "SetCVar"), "command autocomplete should suggest SetCVar");

		const std::vector<std::string> cvarCompletions = commandRegistry.CompleteLine("GetCVar test.", runtimeContext);
		Expect(Contains(cvarCompletions, "test.Bool"), "CVar autocomplete should suggest registered variables");
	}

	void ExpectCommandScopeFiltering(ConsoleCommandRegistry& commandRegistry)
	{
		const bool registered = commandRegistry.Register(ConsoleCommandDescriptor{
		    .Name = "EditorOnly",
		    .Help = "Editor-only command.",
		    .Scope = ConsoleCommandScope::Editor,
		    .Execute = [](const ConsoleCommandContext&, std::span<const std::string_view>) {
			    return ConsoleCommandResult::Success("editor command ran");
		    },
		});
		Expect(registered, "editor-only command should register");

		const ConsoleCommandContext runtimeContext{.Scope = ConsoleCommandScope::Runtime};
		const ConsoleCommandContext editorContext{.Scope = ConsoleCommandScope::Editor};
		Expect(!commandRegistry.ExecuteLine("EditorOnly", runtimeContext).Succeeded, "runtime scope should not execute editor commands");
		Expect(commandRegistry.ExecuteLine("EditorOnly", editorContext).Succeeded, "editor scope should execute editor commands");
	}

	void ExpectConsoleSession(ConsoleCommandRegistry& commandRegistry, ConsoleVariable<int>& intCVar)
	{
		ConsoleSession session(commandRegistry, ConsoleCommandContext{.Scope = ConsoleCommandScope::Runtime});
		session.SubmitLine("SetCVar test.Int 77");
		Expect(intCVar.Get() == 77, "ConsoleSession should dispatch submitted commands");
		Expect(session.GetOutputRecords().size() >= 2, "ConsoleSession should record command echo and command output");

		const std::optional<std::string> previous = session.NavigateHistoryPrevious("");
		Expect(previous && *previous == "SetCVar test.Int 77", "ConsoleSession history should navigate to the last command");

		const std::optional<std::string> next = session.NavigateHistoryNext();
		Expect(next && next->empty(), "ConsoleSession history should restore the pending input after navigating forward");
	}

	void Expect(bool condition, const char* message)
	{
		if (condition)
		{
			return;
		}

		++m_failureCount;
		std::cerr << "[ConsoleDiagnostics] " << message << '\n';
	}

	static bool Contains(const std::vector<std::string>& values, const std::string& expectedValue)
	{
		for (const std::string& value : values)
		{
			if (value == expectedValue)
			{
				return true;
			}
		}
		return false;
	}

	int m_failureCount = 0;
};

int main()
{
	ConsoleDiagnostics diagnostics;
	return diagnostics.Run();
}
