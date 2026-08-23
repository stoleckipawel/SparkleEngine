#pragma once

#include "ShaderRecook/ShaderRecookRequest.h"

#include <filesystem>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

inline constexpr int kShaderCompilerNoWorkExitCode = 2;

struct ShaderCompilerProcessResult final
{
	int ExitCode = -1;
	std::filesystem::path ExecutablePath;
	std::string CommandLine;
	std::string Output;

	bool Succeeded() const noexcept { return ExitCode == 0; }
	bool NoWork() const noexcept { return ExitCode == kShaderCompilerNoWorkExitCode; }
	bool SettledSuccessfully() const noexcept { return Succeeded() || NoWork(); }
};

class ShaderCompilerProcess final
{
public:
	ShaderCompilerProcess() = delete;

	static ShaderCompilerProcessResult RunCook(const ShaderRecookRequest& request, std::stop_token cancellation = {}) noexcept;
	static ShaderCompilerProcessResult RunToolCommand(std::string_view command) noexcept;

private:
	static std::filesystem::path ResolveExecutable() noexcept;
	static std::filesystem::path ResolveProjectDirectory() noexcept;
	static ShaderCompilerProcessResult RunCommand(
	    const std::filesystem::path& executablePath,
	    const std::filesystem::path& workingDirectory,
	    std::vector<std::string> arguments) noexcept;
};
