#include "Cli/CommandRegistry.h"
#include "Cli/ICommand.h"
#include "Constants/ShaderCompilerConstants.h"
#include "RendererShaderRegistration.h"

#include <iostream>
#include <span>
#include <string_view>
#include <vector>

int main(int argc, char** argv)
{
	RegisterRendererGlobalShaders();

	const CommandRegistry registry;

	if (argc >= 2)
	{
		const std::string_view verb{argv[1]};
		if (verb == "--help" || verb == "-h" || verb == "/?")
		{
			registry.PrintUsage(std::cout);
			return kExitCodeSuccess;
		}

		const ICommand* command = registry.Find(verb);
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

	registry.PrintUsage(std::cerr);
	return kExitCodeUsage;
}
