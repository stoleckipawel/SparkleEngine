#include "PCH.h"

#include "ShaderRecook/ShaderCompilerProcess.h"

#include "Core/Public/FileSystemUtils.h"

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
		result.Output = "Projects/Showcase was not found from the current build output.";
		return result;
	}

	std::string arguments = "cook";
	if (request.Type == ShaderRecookRequestType::ShaderPathOrId)
	{
		if (request.Target.empty())
		{
			result.Output = "Targeted shader recook requires a shader source path, package id, or shader id.";
			return result;
		}

		arguments += " --shader ";
		arguments += QuotePath(std::filesystem::path(request.Target));
	}

	arguments += " --debug-artifacts ";
	arguments += QuotePath(ResolveDebugArtifactDirectory());
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
		result.Output = "Projects/Showcase was not found from the current build output.";
		return result;
	}

	return RunCommand(executablePath, projectDirectory, command);
}

std::filesystem::path ShaderCompilerProcess::ResolveRecookSignalPath() noexcept
{
	return Filesystem::GetExecutableDirectory().parent_path() / "Cache" / "Shaders" / "recook.signal";
}

std::filesystem::path ShaderCompilerProcess::ResolveExecutable() noexcept
{
	const std::filesystem::path executableDirectory = Filesystem::GetExecutableDirectory();
	const std::array<std::filesystem::path, 3> candidates{
	    executableDirectory / "ShaderCompiler.exe",
	    executableDirectory.parent_path() / "ShaderCompiler.exe",
	    executableDirectory.parent_path() / "Debug" / "ShaderCompiler.exe"};

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
	const std::filesystem::path repoRoot = Filesystem::GetExecutableDirectory().parent_path().parent_path();
	const std::filesystem::path showcaseDirectory = repoRoot / "Projects" / "Showcase";
	std::error_code errorCode;
	if (std::filesystem::exists(showcaseDirectory, errorCode) && !errorCode)
	{
		return showcaseDirectory;
	}

	return {};
}

std::filesystem::path ShaderCompilerProcess::ResolveDebugArtifactDirectory() noexcept
{
	return Filesystem::GetExecutableDirectory().parent_path() / "Cache" / "Shaders" / "Debug";
}

std::string ShaderCompilerProcess::QuotePath(const std::filesystem::path& path)
{
	std::string text = path.string();
	std::string quoted;
	quoted.reserve(text.size() + 2);
	quoted.push_back('"');
	for (const char ch : text)
	{
		if (ch == '"')
		{
			quoted.push_back('\\');
		}
		quoted.push_back(ch);
	}
	quoted.push_back('"');
	return quoted;
}

ShaderCompilerProcessResult ShaderCompilerProcess::RunCommand(
    const std::filesystem::path& executablePath,
    const std::filesystem::path& workingDirectory,
    std::string_view arguments) noexcept
{
	ShaderCompilerProcessResult result;
	result.ExecutablePath = executablePath;
	const std::string command = std::format(
	    "cd /d {} && {} {} 2>&1",
	    QuotePath(workingDirectory),
	    QuotePath(executablePath),
	    arguments);
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