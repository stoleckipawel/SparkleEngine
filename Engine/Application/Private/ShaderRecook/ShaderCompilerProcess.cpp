#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "ShaderRecook/ShaderCompilerProcess.h"

#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Process/ChildProcess.h"
#include "Core/Public/Process/CommandLineUtils.h"

#include <array>
#include <sstream>
#include <system_error>

class ShaderCompilerCommandPresentation final
{
  public:
	static std::string BuildDisplayCommand(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments)
	{
		std::ostringstream command;
		command << CommandLine::QuotePath(executablePath);
		for (const std::string& argument : arguments)
			command << ' ' << CommandLine::QuoteArgument(argument);
		return command.str();
	}
};

ShaderCompilerProcessResult ShaderCompilerProcess::RunCook(const ShaderRecookRequest& request, std::stop_token cancellation) noexcept
{
	std::vector<std::string> arguments{"cook"};
	if (request.Type == ShaderRecookRequestType::PackageId || request.Type == ShaderRecookRequestType::ShaderId)
	{
		if (request.Target.empty())
			return ShaderCompilerProcessResult{.Output = "Targeted shader recook requires a package id or shader id."};
		arguments.push_back(request.Type == ShaderRecookRequestType::PackageId ? "--package" : "--shader-id");
		arguments.push_back(request.Target);
	}
	return RunCommand(ResolveExecutable(), ResolveProjectDirectory(), std::move(arguments), cancellation);
}

ShaderCompilerProcessResult ShaderCompilerProcess::RunToolCommand(std::string_view command) noexcept
{
	return RunCommand(ResolveExecutable(), ResolveProjectDirectory(), {std::string(command)}, {});
}

std::filesystem::path ShaderCompilerProcess::ResolveExecutable() noexcept
{
	const auto candidates = Paths::ExecutableLookupCandidates("ShaderCompiler.exe");
	std::error_code error;
	for (const std::filesystem::path& candidate : candidates)
	{
		if (std::filesystem::exists(candidate, error) && !error)
			return candidate;
		error.clear();
	}
	return {};
}

std::filesystem::path ShaderCompilerProcess::ResolveProjectDirectory() noexcept
{
	const std::filesystem::path projectDirectory = Filesystem::GetProjectPath();
	std::error_code error;
	return std::filesystem::exists(projectDirectory, error) && !error ? projectDirectory : std::filesystem::path{};
}

ShaderCompilerProcessResult ShaderCompilerProcess::RunCommand(
    const std::filesystem::path& executablePath,
    const std::filesystem::path& workingDirectory,
    std::vector<std::string> arguments,
    std::stop_token cancellation) noexcept
{
	ShaderCompilerProcessResult result;
	if (executablePath.empty())
	{
		result.Output = "ShaderCompiler.exe was not found in the build output.";
		return result;
	}
	if (workingDirectory.empty())
	{
		result.Output = "Configured project root was not found from the current build output.";
		return result;
	}

	result.ExecutablePath = executablePath;
	result.CommandLine = ShaderCompilerCommandPresentation::BuildDisplayCommand(executablePath, arguments);
	Process::ChildProcessResult process = Process::ChildProcess::Run(
	    Process::ChildProcessRequest{
	        .ExecutablePath = executablePath,
	        .Arguments = std::move(arguments),
	        .WorkingDirectory = workingDirectory,
	        .Cancellation = cancellation});
	result.ExitCode = process.ExitCode;
	result.Output = std::move(process.CapturedOutput);
	if (!process.FailureReason.empty())
	{
		if (!result.Output.empty())
			result.Output.push_back('\n');
		result.Output += process.FailureReason;
	}
	if (process.Cancelled)
	{
		if (!result.Output.empty())
			result.Output.push_back('\n');
		result.Output += "Shader compiler process was cancelled.";
	}
	return result;
}
