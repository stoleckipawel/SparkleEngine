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

namespace
{
	ConsoleCommandSeverity ResolveRecookStatusSeverity(const std::string& status) noexcept
	{
		return status.find("failed") != std::string::npos ? ConsoleCommandSeverity::Error : ConsoleCommandSeverity::Info;
	}
}

void ShaderConsoleCommands::Register(ConsoleCommandRegistry& commandRegistry, Handlers handlers)
{
	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "RecompileShaders",
	    .Help = "Queues an out-of-process shader recook. Targets: Global, Changed, or <path-or-id>.",
	    .ArgumentSyntax = "Global|Changed|<path-or-id>",
	    .Scope = ConsoleCommandScope::Editor,
	    .Execute = [handlers](const ConsoleCommandContext&, std::span<const std::string_view> arguments) {
		    return ExecuteRecompileShaders(handlers, arguments);
	    },
	    .Complete = [](const ConsoleCommandContext&, const ConsoleAutocompleteRequest& request) {
		    return CompleteRecompileShaders(request);
	    },
	});

	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "ReloadShaders",
	    .Help = "Reloads currently cooked shader packages without recooking.",
	    .Scope = ConsoleCommandScope::Editor,
	    .Execute = [handlers](const ConsoleCommandContext&, std::span<const std::string_view>) {
		    return ExecuteReloadShaders(handlers);
	    },
	});

	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "ListShaders",
	    .Help = "Lists registered global shaders from the typed shader registry.",
	    .Scope = ConsoleCommandScope::Editor,
	    .Execute = [](const ConsoleCommandContext&, std::span<const std::string_view>) {
		    return ExecuteListShaders();
	    },
	});

	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "ListShaderBackends",
	    .Help = "Lists shader compiler backends mirrored from the tool surface.",
	    .Scope = ConsoleCommandScope::Editor,
	    .Execute = [](const ConsoleCommandContext&, std::span<const std::string_view>) {
		    return ExecuteListShaderBackends();
	    },
	});

	commandRegistry.Register(ConsoleCommandDescriptor{
	    .Name = "ListShaderTargets",
	    .Help = "Lists shader target names accepted by the shader compiler.",
	    .Scope = ConsoleCommandScope::Editor,
	    .Execute = [](const ConsoleCommandContext&, std::span<const std::string_view>) {
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
			    consoleSystem->AppendOutput(ConsoleOutputRecord{.Severity = ResolveRecookStatusSeverity(status), .Text = status});
		    }
		    ui.SetShaderRecookStatus(std::move(status));
	    });

	if (EditorConsoleSystem* consoleSystem = ui.GetEditorConsoleSystem())
	{
		Register(
		    consoleSystem->GetCommandRegistry(),
		    Handlers{
		        .RequestRecook = [&coordinator](ShaderRecookRequest request)
		        {
			        coordinator.RequestRecook(std::move(request));
		        },
		        .RequestReload = [&coordinator]()
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
	else
	{
		request.Type = ShaderRecookRequestType::ShaderPathOrId;
		request.Target = Strings::Join(arguments, " ");
	}

	handlers.RequestRecook(request);
	return ConsoleCommandResult::Success("queued " + ShaderRecookCoordinator::DescribeRequest(request) + " through out-of-process ShaderCompiler.exe cook");
}

ConsoleCommandResult ShaderConsoleCommands::ExecuteReloadShaders(const Handlers& handlers)
{
	if (!handlers.RequestReload)
	{
		return ConsoleCommandResult::Error("shader reload service is unavailable");
	}

	handlers.RequestReload();
	return ConsoleCommandResult::Success("queued cooked shader package reload");
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
	std::vector<std::string> completions = BuildShaderTargetCompletions(prefix);
	if (Strings::StartsWithIgnoreCase("Global", prefix))
	{
		completions.emplace_back("Global");
	}
	if (Strings::StartsWithIgnoreCase("Changed", prefix))
	{
		completions.emplace_back("Changed");
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
		const ShaderPermutationDomainDescriptor permutations =
		    shader.BuildPermutationDomainDescriptor != nullptr ? shader.BuildPermutationDomainDescriptor() : ShaderPermutationDomainDescriptor{};

		if (!output.empty())
		{
			output += '\n';
		}
		output += std::format(
		    "{} package={} stage={} source={} entry={} parameters={} permutationDimensions={}",
		    shader.ShaderName,
		    shader.PackageName.empty() ? shader.ShaderName : shader.PackageName,
		    GetShaderStagePrefix(shader.Stage),
		    shader.SourcePath,
		    shader.EntryPoint,
		    parameters.Fields.size(),
		    permutations.Dimensions.size());
	}
	return output;
}

std::vector<std::string> ShaderConsoleCommands::BuildShaderTargetCompletions(std::string_view prefix)
{
	std::set<std::string> uniqueCompletions;
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const std::string shaderName(shader.ShaderName);
		const std::string packageName(shader.PackageName.empty() ? shader.ShaderName : shader.PackageName);
		const std::string sourcePath(shader.SourcePath);
		if (Strings::StartsWithIgnoreCase(shaderName, prefix))
		{
			uniqueCompletions.insert(shaderName);
		}
		if (Strings::StartsWithIgnoreCase(packageName, prefix))
		{
			uniqueCompletions.insert(packageName);
		}
		if (Strings::StartsWithIgnoreCase(sourcePath, prefix))
		{
			uniqueCompletions.insert(sourcePath);
		}
	}
	return {uniqueCompletions.begin(), uniqueCompletions.end()};
}
