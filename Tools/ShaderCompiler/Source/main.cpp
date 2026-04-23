#include "Cli/CommandRegistry.h"
#include "Cli/ICommand.h"
#include "Constants/ShaderCompilerConstants.h"

#include <iostream>
#include <span>
#include <string_view>
#include <vector>

int main(int argc, char** argv)
{
	const CommandRegistry registry;

	if (argc >= 2)
	{
		const ICommand* command = registry.Find(std::string_view{argv[1]});
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
