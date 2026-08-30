#include "AssetCookerToolProcess.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Process/ChildProcess.h"
#include "ToolConsole.h"

#include <iostream>

int AssetCookerToolProcess::Run(
    const std::filesystem::path& executablePath,
    const std::vector<std::string>& arguments,
    const std::filesystem::path& workingDirectory,
    std::stop_token cancellation)
{
	static const auto toolProcessLogger = Logging::GetOrCreateLogger("Tools.AssetCooker.Process");

	Process::ChildProcessResult result = Process::ChildProcess::Run(
	    Process::ChildProcessRequest{
	        .ExecutablePath = executablePath,
	        .Arguments = arguments,
	        .WorkingDirectory = workingDirectory,
	        .OutputCallback = [](std::string_view output) { std::cout.write(output.data(), static_cast<std::streamsize>(output.size())); },
	        .Cancellation = cancellation});
	if (!result.Launched)
	{
		ToolConsole::Message(
		    std::cerr,
		    ToolConsoleSeverity::Error,
		    "Failed to launch tool",
		    {ToolConsole::QuotedField("tool", executablePath.filename().string()),
		        ToolConsole::PathField("path", executablePath),
		        ToolConsole::QuotedField("reason", result.FailureReason)});
		SPDLOG_LOGGER_ERROR(toolProcessLogger, "Failed to launch process '{}': {}", executablePath.string(), result.FailureReason);
		return 1;
	}
	return result.ExitCode;
}
