#pragma once

#include "Core/Public/Process/ChildProcess.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	using ProcessOutputCallback = std::function<void(std::string_view)>;

	struct ProcessRequest
	{
		std::filesystem::path ExecutablePath;
		std::vector<std::string> Arguments;
		std::filesystem::path WorkingDirectory;
		std::vector<Process::EnvironmentOverride> Environment;
		std::filesystem::path LogPath;
		ProcessOutputCallback OutputCallback;
		std::stop_token Cancellation;
	};

	struct ProcessResult
	{
		bool Launched = false;
		bool Canceled = false;
		int ExitCode = -1;
		std::string CapturedOutput;
		std::string FailureReason;
		std::chrono::system_clock::time_point StartTime = {};
		std::chrono::system_clock::time_point EndTime = {};
	};

	class IProcessRunner
	{
	public:
		virtual ~IProcessRunner() = default;
		virtual ProcessResult Run(const ProcessRequest& request) = 0;
	};

	class NativeProcessRunner final : public IProcessRunner
	{
	public:
		ProcessResult Run(const ProcessRequest& request) override;
	};

	std::string BuildDisplayCommandLine(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments);
}
