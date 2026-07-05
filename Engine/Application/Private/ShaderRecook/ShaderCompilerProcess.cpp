#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "ShaderRecook/ShaderCompilerProcess.h"

#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Process/CommandLineUtils.h"

#include <array>
#include <cstdio>
#include <format>
#include <sstream>
#include <system_error>

ShaderCompilerProcessResult ShaderCompilerProcess::RunCook(const ShaderRecookRequest& request) noexcept
{
	ShaderCompilerProcessResult result;
	const std::filesystem::path executablePath = ResolveExecutable();
	if (executablePath.empty())
	{
		result.Output = "ShaderCompiler.exe was not found in the build output.";
		return result;
	}

	const std::filesystem::path projectDirectory = ResolveProjectDirectory();
	if (projectDirectory.empty())
	{
		result.Output = "Configured project root was not found from the current build output.";
		return result;
	}

	std::string arguments = "cook";
	if (request.Type == ShaderRecookRequestType::PackageId || request.Type == ShaderRecookRequestType::ShaderId)
	{
		if (request.Target.empty())
		{
			result.Output = "Targeted shader recook requires a package id or shader id.";
			return result;
		}

		arguments += request.Type == ShaderRecookRequestType::PackageId ? " --package " : " --shader-id ";
		arguments += CommandLine::QuotePath(std::filesystem::path(request.Target));
	}

	arguments += " --debug-artifacts ";
	arguments += CommandLine::QuotePath(Filesystem::GetShaderDebugArtifactRootPath());
	return RunCommand(executablePath, projectDirectory, arguments);
}

ShaderCompilerProcessResult ShaderCompilerProcess::RunToolCommand(std::string_view command) noexcept
{
	ShaderCompilerProcessResult result;
	const std::filesystem::path executablePath = ResolveExecutable();
	if (executablePath.empty())
	{
		result.Output = "ShaderCompiler.exe was not found in the build output.";
		return result;
	}

	const std::filesystem::path projectDirectory = ResolveProjectDirectory();
	if (projectDirectory.empty())
	{
		result.Output = "Configured project root was not found from the current build output.";
		return result;
	}

	return RunCommand(executablePath, projectDirectory, command);
}

std::filesystem::path ShaderCompilerProcess::ResolveExecutable() noexcept
{
	const std::array<std::filesystem::path, 3> candidates = Paths::ExecutableLookupCandidates("ShaderCompiler.exe");

	std::error_code errorCode;
	for (const std::filesystem::path& candidate : candidates)
	{
		if (std::filesystem::exists(candidate, errorCode) && !errorCode)
		{
			return candidate;
		}
		errorCode.clear();
	}

	return {};
}

std::filesystem::path ShaderCompilerProcess::ResolveProjectDirectory() noexcept
{
	const std::filesystem::path showcaseDirectory = Filesystem::GetProjectPath();
	std::error_code errorCode;
	if (std::filesystem::exists(showcaseDirectory, errorCode) && !errorCode)
	{
		return showcaseDirectory;
	}

	return {};
}

ShaderCompilerProcessResult ShaderCompilerProcess::RunCommand(
    const std::filesystem::path& executablePath,
    const std::filesystem::path& workingDirectory,
    std::string_view arguments) noexcept
{
	ShaderCompilerProcessResult result;
	result.ExecutablePath = executablePath;
	const std::string command =
	    std::format("cd /d {} && {} {} 2>&1", CommandLine::QuotePath(workingDirectory), CommandLine::QuotePath(executablePath), arguments);
	result.CommandLine = command;

	FILE* pipe = _popen(command.c_str(), "r");
	if (pipe == nullptr)
	{
		result.Output = "Failed to launch ShaderCompiler.exe.";
		return result;
	}

	std::array<char, 4096> buffer{};
	std::ostringstream output;
	while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
	{
		output << buffer.data();
	}

	result.ExitCode = _pclose(pipe);
	result.Output = output.str();
	return result;
}
