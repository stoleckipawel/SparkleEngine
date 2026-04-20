#include "Cli/CommandRegistry.h"
#include "Cli/ICommand.h"
#include "Constants/ShaderCompilerConstants.h"

#include <iostream>
#include <string_view>

int main(int argc, char** argv)
{
	const CommandRegistry registry;

	constexpr int kExpectedArgCount = 2;
	if (argc == kExpectedArgCount)
	{
		const ICommand* command = registry.Find(std::string_view{argv[1]});
		if (command != nullptr)
		{
			return command->Run();
		}
	}

	registry.PrintUsage(std::cerr);
	return kExitCodeUsage;
}
