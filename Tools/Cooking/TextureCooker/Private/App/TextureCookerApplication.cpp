#include "PCH.h"

#include "App/TextureCookerApplication.h"

#include "Cli/TextureCookerCommandRegistry.h"
#include "Constants/TextureCookerConstants.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/ScopedLogEvent.h"
#include "Core/Public/Diagnostics/Trace.h"

#include <iostream>
#include <memory>
#include <string_view>

	int TextureCookerApplication::Run(int argc, char** argv) const
	{
		static const auto textureCookerLogger = Logging::GetOrCreateLogger("Tools.TextureCooker");
		SPARKLE_CPU_SCOPE("Tools.TextureCooker.Application.Run");
		SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, "TextureCooker.Application.Run");

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
		static const auto textureCookerLogger = Logging::GetOrCreateLogger("Tools.TextureCooker");
		SPARKLE_CPU_SCOPE("Tools.TextureCooker.Application.RunCommand");
		SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, "TextureCooker.Application.RunCommand");

		const std::string_view command(commandName != nullptr ? commandName : "");
		SPDLOG_LOGGER_INFO(
			textureCookerLogger,
			"TextureCooker command='{}' requestFile='{}' summaryPath='{}'",
			command,
			requestFilePath.string(),
			summaryPath.string());
		std::unique_ptr<TextureCookerCommand> commandHandler = TextureCookerCommandRegistry::Create(command);
		if (!commandHandler)
		{
			TextureCookerCommandRegistry::PrintUsage(std::cerr);
			return TextureCookerConstants::ExitUsageError;
		}

		TextureCookerCommandOptions options;
		options.summaryPath = summaryPath;
		const int exitCode = commandHandler->Execute(requestFilePath, options);
		SPDLOG_LOGGER_INFO(textureCookerLogger, "TextureCooker command='{}' completed with exitCode={}", command, exitCode);
		return exitCode;
	}