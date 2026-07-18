#include "SparkleLauncher/ProcessRunner.h"

#include <sstream>

namespace SparkleLauncher
{
	namespace
	{
		std::string QuoteDisplayArgument(std::string_view argument)
		{
			if (argument.empty())
				return "\"\"";
			const bool needsQuotes = argument.find_first_of(" \t\"") != std::string_view::npos;
			if (!needsQuotes)
				return std::string(argument);
			std::string quoted(1, '"');
			for (const char character : argument)
			{
				if (character == '"')
					quoted.push_back('\\');
				quoted.push_back(character);
			}
			quoted.push_back('"');
			return quoted;
		}
	}

	ProcessResult NativeProcessRunner::Run(const ProcessRequest& request)
	{
		ProcessResult result;
		result.StartTime = std::chrono::system_clock::now();
		Process::ChildProcessResult childResult = Process::ChildProcess::Run(Process::ChildProcessRequest{
		    .ExecutablePath = request.ExecutablePath,
		    .Arguments = request.Arguments,
		    .WorkingDirectory = request.WorkingDirectory,
		    .Environment = request.Environment,
		    .LogPath = request.LogPath,
		    .OutputCallback = request.OutputCallback,
		    .Cancellation = request.Cancellation});
		result.Launched = childResult.Launched;
		result.Canceled = childResult.Cancelled;
		result.ExitCode = childResult.ExitCode;
		result.CapturedOutput = std::move(childResult.CapturedOutput);
		result.FailureReason = std::move(childResult.FailureReason);
		result.EndTime = std::chrono::system_clock::now();
		return result;
	}

	std::string BuildDisplayCommandLine(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments)
	{
		std::ostringstream commandLine;
		commandLine << QuoteDisplayArgument(executablePath.string());
		for (const std::string& argument : arguments)
			commandLine << ' ' << QuoteDisplayArgument(argument);
		return commandLine.str();
	}
}
