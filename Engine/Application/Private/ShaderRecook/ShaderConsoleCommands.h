#pragma once

#include "ShaderRecook/ShaderRecookRequest.h"

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class ConsoleCommandRegistry;
class ShaderRecookCoordinator;
class UI;
struct ConsoleAutocompleteRequest;
struct ConsoleCommandResult;

class ShaderConsoleCommands final
{
public:
	struct Handlers final
	{
		std::function<void(ShaderRecookRequest)> RequestRecook;
		std::function<void()> RequestReload;
	};

	static void Register(ConsoleCommandRegistry& commandRegistry, const Handlers& handlers);
	static void ConnectEditor(UI& ui, ShaderRecookCoordinator& coordinator);

private:
	static ConsoleCommandResult ExecuteRecompileShaders(const Handlers& handlers, std::span<const std::string_view> arguments);
	static ConsoleCommandResult ExecuteReloadShaders(const Handlers& handlers);
	static ConsoleCommandResult ExecuteListShaders();
	static ConsoleCommandResult ExecuteListShaderBackends();
	static ConsoleCommandResult ExecuteListShaderTargets();
	static std::vector<std::string> CompleteRecompileShaders(const ConsoleAutocompleteRequest& request);
	static std::string BuildShaderList();
	static std::vector<std::string> BuildShaderIdCompletions(std::string_view prefix);
};
