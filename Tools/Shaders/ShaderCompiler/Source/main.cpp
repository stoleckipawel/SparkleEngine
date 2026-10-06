#include "Cli/ShaderCompilerCommands.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Threading/ThreadOwnership.h"

#include <iostream>
#include <span>
#include <string_view>
#include <vector>

int main(int argc, char** argv)
{
	Threading::SetCurrentThreadRole("Sparkle.ToolMain");

	if (argc >= 2)
	{
		const std::string_view verb{argv[1]};
		if (verb == "--help" || verb == "-h" || verb == "/?")
		{
			PrintShaderCompilerUsage(std::cout);
			return kExitCodeSuccess;
		}

		const ShaderCompilerCommand* command = FindShaderCompilerCommand(verb);
		if (command != nullptr)
		{
			std::vector<std::string_view> commandArgs;
			commandArgs.reserve(static_cast<std::size_t>(argc - 2));
			for (int index = 2; index < argc; ++index)
			{
				commandArgs.emplace_back(argv[index]);
			}

			return command->Run(commandArgs);
		}
	}

	PrintShaderCompilerUsage(std::cerr);
	return kExitCodeUsage;
}
