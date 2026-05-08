#include "PCH.h"

#include "App/TextureCookerApplication.h"

#include "Cli/TextureCookerCommandRegistry.h"
#include "Constants/TextureCookerConstants.h"

#include <iostream>
#include <memory>
#include <string_view>

namespace AssetAuthoring
{
	int TextureCookerApplication::Run(int argc, char** argv) const
	{
		if (argc != 3)
		{
			TextureCookerCommandRegistry::PrintUsage(std::cerr);
			return TextureCookerConstants::ExitUsageError;
		}

		return RunCommand(std::filesystem::path(argv[2]), argv[1]);
	}

	int TextureCookerApplication::RunCommand(const std::filesystem::path& requestFilePath, const char* commandName)
	{
		const std::string_view command(commandName != nullptr ? commandName : "");
		std::unique_ptr<TextureCookerCommand> commandHandler = TextureCookerCommandRegistry::Create(command);
		if (!commandHandler)
		{
			TextureCookerCommandRegistry::PrintUsage(std::cerr);
			return TextureCookerConstants::ExitUsageError;
		}

		return commandHandler->Execute(requestFilePath);
	}
}