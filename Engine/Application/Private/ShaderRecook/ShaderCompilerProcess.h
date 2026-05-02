#pragma once

#include "ShaderRecook/ShaderRecookRequest.h"

#include <filesystem>
#include <string>
#include <string_view>

struct ShaderCompilerProcessResult final
{
	int ExitCode = -1;
	std::filesystem::path ExecutablePath;
	std::string CommandLine;
	std::string Output;

	bool Succeeded() const noexcept { return ExitCode == 0; }
};

class ShaderCompilerProcess final
{
  public:
	ShaderCompilerProcess() = delete;

	static ShaderCompilerProcessResult RunCook(const ShaderRecookRequest& request) noexcept;
	static ShaderCompilerProcessResult RunToolCommand(std::string_view command) noexcept;

  private:
	static std::filesystem::path ResolveExecutable() noexcept;
	static std::filesystem::path ResolveProjectDirectory() noexcept;
	static ShaderCompilerProcessResult RunCommand(
	    const std::filesystem::path& executablePath,
	    const std::filesystem::path& workingDirectory,
	    std::string_view arguments) noexcept;
};