#include "PCH.h"

#include "Cli/ShaderCompilerCommands.h"

#include "Cli/CookShadersCommand.h"
#include "Cli/InspectShaderCommand.h"
#include "Cli/ListBackendsCommand.h"
#include "Cli/ListShadersCommand.h"
#include "Cli/ListTargetsCommand.h"
#include "Constants/ShaderCompilerConstants.h"

#include <array>

static constexpr auto commands = std::to_array<ShaderCompilerCommand>(
    {{kCommandCook,
         &CookShaders,
         "  ShaderCompiler cook [--shader-id <registered-shader-name> | --changed "
         "<virtual-path>...] [--target <name>] [--backend <name>] [--debug-artifacts <dir>] [--analysis <pass>] "
         "[--debug-info] [--disable-optimizations]"},
        {kCommandListBackends, &ListShaderBackends, "  ShaderCompiler list-backends"},
        {kCommandListTargets, &ListShaderTargets, "  ShaderCompiler list-targets"},
        {kCommandListShaders, &ListShaders, "  ShaderCompiler list-shaders [--validate]"},
        {kCommandInspectShader, &InspectShader, "  ShaderCompiler inspect-shader <shader-id>"}});

const ShaderCompilerCommand* FindShaderCompilerCommand(std::string_view verb) noexcept
{
	for (const ShaderCompilerCommand& command : commands)
	{
		if (command.Verb == verb)
		{
			return &command;
		}
	}
	return nullptr;
}

void PrintShaderCompilerUsage(std::ostream& output)
{
	output << "Usage:\n";
	for (const ShaderCompilerCommand& command : commands)
	{
		output << command.Usage << '\n';
	}
}
