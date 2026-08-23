#include "PCH.h"

#include "ShaderRecook/ShaderConsoleCommands.h"

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleOutput.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Editor/Public/Console/EditorConsoleSystem.h"
#include "Editor/Public/UI.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "RHI/Public/Shaders/ShaderStage.h"
#include "ShaderRecook/ShaderCompilerProcess.h"
#include "ShaderRecook/ShaderRecookCoordinator.h"

#include <format>
#include <set>
#include <utility>

class ShaderRecookStatusPresentation final
{
  public:
	static ConsoleCommandSeverity ResolveRecookStatusSeverity(const std::string& status) noexcept
	{
		return status.find("failed") != std::string::npos ? ConsoleCommandSeverity::Error : ConsoleCommandSeverity::Info;
	}
};

void ShaderConsoleCommands::Register(ConsoleCommandRegistry& commandRegistry, Handlers handlers)
{
	commandRegistry.Register(
	    ConsoleCommandDescriptor{
	        .Name = "RecompileShaders",
	        .Help = "Queues an out-of-process shader recook. Targets: Global, Changed, or Shader <shader-id>.",
	        .ArgumentSyntax = "Global|Changed|Shader <shader-id>",
	        .Scope = ConsoleCommandScope::Editor,
	        .Execute =
	            [handlers](ConsoleCommandScope, std::span<const std::string_view> arguments)
	        {
		        return ExecuteRecompileShaders(handlers, arguments);
	        },
	        .Complete =
	            [](ConsoleCommandScope, const ConsoleAutocompleteRequest& request)
	        {
		        return CompleteRecompileShaders(request);
	        },
	    });

	commandRegistry.Register(
	    ConsoleCommandDescriptor{
	        .Name = "ReloadShaders",
	        .Help = "Reloads the current cooked shader map and code library without recooking.",
	        .Scope = ConsoleCommandScope::Editor,
	        .Execute =
	            [handlers](ConsoleCommandScope, std::span<const std::string_view>)
	        {
		        return ExecuteReloadShaders(handlers);
	        },
	    });

	commandRegistry.Register(
	    ConsoleCommandDescriptor{
	        .Name = "ListShaders",
	        .Help = "Lists registered global shaders from the typed shader registry.",
	        .Scope = ConsoleCommandScope::Editor,
	        .Execute =
	            [](ConsoleCommandScope, std::span<const std::string_view>)
	        {
		        return ExecuteListShaders();
	        },
	    });

	commandRegistry.Register(
	    ConsoleCommandDescriptor{
	        .Name = "ListShaderBackends",
	        .Help = "Lists shader compiler backends mirrored from the tool surface.",
	        .Scope = ConsoleCommandScope::Editor,
	        .Execute =
	            [](ConsoleCommandScope, std::span<const std::string_view>)
	        {
		        return ExecuteListShaderBackends();
	        },
	    });

	commandRegistry.Register(
	    ConsoleCommandDescriptor{
	        .Name = "ListShaderTargets",
	        .Help = "Lists shader target names accepted by the shader compiler.",
	        .Scope = ConsoleCommandScope::Editor,
	        .Execute =
	            [](ConsoleCommandScope, std::span<const std::string_view>)
	        {
		        return ExecuteListShaderTargets();
	        },
	    });
}

void ShaderConsoleCommands::ConnectEditor(UI& ui, ShaderRecookCoordinator& coordinator)
{
	coordinator.SetStatusHandler(
	    [&ui](std::string status)
	    {
		    if (EditorConsoleSystem* consoleSystem = ui.GetEditorConsoleSystem())
		    {
			    const ConsoleCommandSeverity severity = ShaderRecookStatusPresentation::ResolveRecookStatusSeverity(status);
			    consoleSystem->AppendOutput(ConsoleOutputRecord{.Severity = severity, .Text = std::move(status)});
			    consoleSystem->OpenConsole();
		    }
	    });

	if (EditorConsoleSystem* consoleSystem = ui.GetEditorConsoleSystem())
	{
		Register(
		    consoleSystem->GetCommandRegistry(),
		    Handlers{
		        .RequestRecook =
		            [&coordinator](ShaderRecookRequest request)
		        {
			        coordinator.RequestRecook(std::move(request));
		        },
		        .RequestReload =
		            [&coordinator]()
		        {
			        coordinator.RequestReload();
		        },
		    });
	}
}

ConsoleCommandResult ShaderConsoleCommands::ExecuteRecompileShaders(const Handlers& handlers, std::span<const std::string_view> arguments)
{
	if (!handlers.RequestRecook)
	{
		return ConsoleCommandResult::Error("shader recook service is unavailable");
	}

	ShaderRecookRequest request{};
	if (arguments.empty() || Strings::EqualsIgnoreCase(arguments.front(), "Global"))
	{
		request.Type = ShaderRecookRequestType::Global;
	}
	else if (Strings::EqualsIgnoreCase(arguments.front(), "Changed"))
	{
		request.Type = ShaderRecookRequestType::Changed;
	}
	else if (Strings::EqualsIgnoreCase(arguments.front(), "Shader"))
	{
		if (arguments.size() < 2)
		{
			return ConsoleCommandResult::Error("RecompileShaders Shader requires a shader id");
		}
		request.Type = ShaderRecookRequestType::ShaderId;
		request.Target = Strings::Join(arguments.subspan(1), " ");
	}
	else
	{
		return ConsoleCommandResult::Error("Use RecompileShaders Global, Changed, or Shader <shader-id>");
	}

	handlers.RequestRecook(request);
	return ConsoleCommandResult::Success(
	    "queued " + ShaderRecookCoordinator::DescribeRequest(request) + " through out-of-process ShaderCompiler.exe cook");
}

ConsoleCommandResult ShaderConsoleCommands::ExecuteReloadShaders(const Handlers& handlers)
{
	if (!handlers.RequestReload)
	{
		return ConsoleCommandResult::Error("shader reload service is unavailable");
	}

	handlers.RequestReload();
	return ConsoleCommandResult::Success("queued cooked shader map reload");
}

ConsoleCommandResult ShaderConsoleCommands::ExecuteListShaders()
{
	std::string output = BuildShaderList();
	if (output.empty())
	{
		return ConsoleCommandResult::Warning("no typed global shaders are registered");
	}
	return ConsoleCommandResult::Success(std::move(output));
}

ConsoleCommandResult ShaderConsoleCommands::ExecuteListShaderBackends()
{
	ShaderCompilerProcessResult result = ShaderCompilerProcess::RunToolCommand("list-backends");
	if (!result.Succeeded())
	{
		return ConsoleCommandResult::Error(result.Output.empty() ? "failed to query shader compiler backends" : std::move(result.Output));
	}
	return ConsoleCommandResult::Success(std::move(result.Output));
}

ConsoleCommandResult ShaderConsoleCommands::ExecuteListShaderTargets()
{
	ShaderCompilerProcessResult result = ShaderCompilerProcess::RunToolCommand("list-targets");
	if (!result.Succeeded())
	{
		return ConsoleCommandResult::Error(result.Output.empty() ? "failed to query shader compiler targets" : std::move(result.Output));
	}
	return ConsoleCommandResult::Success(std::move(result.Output));
}

std::vector<std::string> ShaderConsoleCommands::CompleteRecompileShaders(const ConsoleAutocompleteRequest& request)
{
	const std::string_view prefix = request.CurrentToken;
	if (!request.Arguments.empty() && Strings::EqualsIgnoreCase(request.Arguments.front(), "Shader"))
	{
		return BuildShaderIdCompletions(prefix);
	}

	std::vector<std::string> completions;
	if (Strings::StartsWithIgnoreCase("Global", prefix))
	{
		completions.emplace_back("Global");
	}
	if (Strings::StartsWithIgnoreCase("Changed", prefix))
	{
		completions.emplace_back("Changed");
	}
	if (Strings::StartsWithIgnoreCase("Shader", prefix))
	{
		completions.emplace_back("Shader");
	}
	return completions;
}

std::string ShaderConsoleCommands::BuildShaderList()
{
	std::string output;
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const ShaderParameterStructDescriptor parameters =
		    shader.BuildParameterStructDescriptor != nullptr ? shader.BuildParameterStructDescriptor() : ShaderParameterStructDescriptor{};

		if (!output.empty())
		{
			output += '\n';
		}
		output += std::format(
		    "{} type={:016x} stage={} source={} entry={} parameters={}",
		    shader.ShaderName,
		    shader.TypeId,
		    GetShaderStagePrefix(shader.Stage),
		    shader.SourcePath,
		    shader.EntryPoint,
		    parameters.Fields.size());
	}
	return output;
}

std::vector<std::string> ShaderConsoleCommands::BuildShaderIdCompletions(std::string_view prefix)
{
	std::set<std::string> uniqueCompletions;
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const std::string shaderName(shader.ShaderName);
		if (Strings::StartsWithIgnoreCase(shaderName, prefix))
		{
			uniqueCompletions.insert(shaderName);
		}
	}
	return {uniqueCompletions.begin(), uniqueCompletions.end()};
}
