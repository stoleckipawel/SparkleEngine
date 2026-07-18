#pragma once

#include "Core/Public/CoreAPI.h"

#include <filesystem>
#include <functional>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

namespace Process
{
	struct EnvironmentOverride final
	{
		std::string Name;
		std::string Value;
	};

	using ChildProcessOutputCallback = std::function<void(std::string_view)>;

	struct ChildProcessRequest final
	{
		std::filesystem::path ExecutablePath;
		std::vector<std::string> Arguments;
		std::filesystem::path WorkingDirectory;
		std::vector<EnvironmentOverride> Environment;
		std::filesystem::path LogPath;
		ChildProcessOutputCallback OutputCallback;
		std::stop_token Cancellation;
	};

	struct ChildProcessResult final
	{
		bool Launched = false;
		bool Cancelled = false;
		int ExitCode = -1;
		std::string CapturedOutput;
		std::string FailureReason;

		bool Succeeded() const noexcept { return Launched && !Cancelled && ExitCode == 0 && FailureReason.empty(); }
	};

	class SPARKLE_CORE_API ChildProcess final
	{
	  public:
		ChildProcess() = delete;
		static ChildProcessResult Run(const ChildProcessRequest& request);
	};
}
