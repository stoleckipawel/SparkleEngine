#include "PCH.h"

#include "App/TextureCookerApplication.h"

#include "Cli/TextureCookerCommandRegistry.h"
#include "Constants/TextureCookerConstants.h"

#include <iostream>
#include <memory>
#include <string_view>

	int TextureCookerApplication::Run(int argc, char** argv) const
	{
		if (argc != 3 && argc != 5)
		{
			TextureCookerCommandRegistry::PrintUsage(std::cerr);
			return TextureCookerConstants::ExitUsageError;
		}

		std::filesystem::path summaryPath;
		if (argc == 5)
		{
			const std::string_view summaryArgument(argv[3]);
			if (summaryArgument != "--summary")
			{
				TextureCookerCommandRegistry::PrintUsage(std::cerr);
				return TextureCookerConstants::ExitUsageError;
			}
			summaryPath = argv[4];
		}

		return RunCommand(std::filesystem::path(argv[2]), argv[1], summaryPath);
	}

	int TextureCookerApplication::RunCommand(
		const std::filesystem::path& requestFilePath,
		const char* commandName,
		const std::filesystem::path& summaryPath)
	{
		const std::string_view command(commandName != nullptr ? commandName : "");
		std::unique_ptr<TextureCookerCommand> commandHandler = TextureCookerCommandRegistry::Create(command);
		if (!commandHandler)
		{
			TextureCookerCommandRegistry::PrintUsage(std::cerr);
			return TextureCookerConstants::ExitUsageError;
		}

		TextureCookerCommandOptions options;
		options.summaryPath = summaryPath;
		const int exitCode = commandHandler->Execute(requestFilePath, options);
		return exitCode;
	}
