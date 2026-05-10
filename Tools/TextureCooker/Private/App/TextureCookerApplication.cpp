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

		if (argc != 3)
		{
			TextureCookerCommandRegistry::PrintUsage(std::cerr);
			return TextureCookerConstants::ExitUsageError;
		}

		return RunCommand(std::filesystem::path(argv[2]), argv[1]);
	}

	int TextureCookerApplication::RunCommand(const std::filesystem::path& requestFilePath, const char* commandName)
	{
		static const auto textureCookerLogger = Logging::GetOrCreateLogger("Tools.TextureCooker");
		SPARKLE_CPU_SCOPE("Tools.TextureCooker.Application.RunCommand");
		SPARKLE_LOG_SCOPE(textureCookerLogger, spdlog::level::info, "TextureCooker.Application.RunCommand");

		const std::string_view command(commandName != nullptr ? commandName : "");
		SPDLOG_LOGGER_INFO(textureCookerLogger, "TextureCooker command='{}' requestFile='{}'", command, requestFilePath.string());
		std::unique_ptr<TextureCookerCommand> commandHandler = TextureCookerCommandRegistry::Create(command);
		if (!commandHandler)
		{
			TextureCookerCommandRegistry::PrintUsage(std::cerr);
			return TextureCookerConstants::ExitUsageError;
		}

		const int exitCode = commandHandler->Execute(requestFilePath);
		SPDLOG_LOGGER_INFO(textureCookerLogger, "TextureCooker command='{}' completed with exitCode={}", command, exitCode);
		return exitCode;
	}