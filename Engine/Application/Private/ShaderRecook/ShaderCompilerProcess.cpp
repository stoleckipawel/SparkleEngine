#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "ShaderRecook/ShaderCompilerProcess.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Process/ChildProcess.h"
#include "Core/Public/Process/CommandLineUtils.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <sstream>
#include <system_error>

class ShaderCompilerCommandPresentation final
{
public:
	static std::string BuildDisplayCommand(const std::filesystem::path& executablePath, const std::vector<std::string>& arguments)
	{
		std::ostringstream command;
		command << CommandLine::QuotePath(executablePath);
		for (std::size_t argumentIndex = 0; argumentIndex < arguments.size(); ++argumentIndex)
		{
			if (arguments[argumentIndex] == "--cancellation-signal")
			{
				++argumentIndex;
				continue;
			}
			command << ' ' << CommandLine::QuoteArgument(arguments[argumentIndex]);
		}
		return command.str();
	}
};

class ShaderCompilerCancellationSignal final
{
public:
	explicit ShaderCompilerCancellationSignal(std::stop_token cancellation) :
	    m_path(BuildPath()),
	    m_callback(cancellation, [this] { Signal(); })
	{
	}

	~ShaderCompilerCancellationSignal()
	{
		Files::CleanupTemporaryFile(m_path);
		std::error_code errorCode;
		std::filesystem::remove(m_path.parent_path(), errorCode);
	}

	const std::filesystem::path& GetPath() const noexcept { return m_path; }

private:
	void Signal() const noexcept
	{
		if (m_path.empty())
		{
			return;
		}
		std::string fileError;
		(void) Files::TryWriteAllTextAtomic(m_path, "cancelled\n", fileError);
	}

	static std::filesystem::path BuildPath()
	{
		std::error_code errorCode;
		const std::filesystem::path temporaryDirectory = std::filesystem::temp_directory_path(errorCode);
		if (errorCode)
		{
			return {};
		}

		static std::atomic_uint64_t sequence = 0;
		const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
		for (std::uint32_t attempt = 0; attempt < 64; ++attempt)
		{
			const std::filesystem::path signalDirectory = temporaryDirectory
			    / std::format("SparkleShaderCookCancellation-{}-{}", ticks, sequence.fetch_add(1, std::memory_order_relaxed));
			errorCode.clear();
			if (std::filesystem::create_directory(signalDirectory, errorCode))
			{
				return signalDirectory / "cancelled.signal";
			}
			if (errorCode)
			{
				return {};
			}
		}
		return {};
	}

	std::filesystem::path m_path;
	std::stop_callback<std::function<void()>> m_callback;
};

ShaderCompilerProcessResult ShaderCompilerProcess::RunCook(const ShaderRecookRequest& request, std::stop_token cancellation) noexcept
{
	ShaderCompilerCancellationSignal cancellationSignal(cancellation);
	if (cancellationSignal.GetPath().empty())
	{
		return ShaderCompilerProcessResult{.Output = "Failed to create the shader cooker cancellation signal path."};
	}

	std::vector<std::string> arguments{"cook"};
	arguments.emplace_back("--cancellation-signal");
	arguments.push_back(cancellationSignal.GetPath().string());
	if (request.Type == ShaderRecookRequestType::ShaderId)
	{
		if (request.Target.empty())
			return ShaderCompilerProcessResult{.Output = "Targeted shader recook requires a shader id."};
		arguments.push_back("--shader-id");
		arguments.push_back(request.Target);
	}
	else if (request.Type == ShaderRecookRequestType::Changed)
	{
		if (request.ChangedVirtualPaths.empty())
		{
			return ShaderCompilerProcessResult{.Output = "Changed shader recook requires at least one virtual source path."};
		}
		for (const std::string& virtualPath : request.ChangedVirtualPaths)
		{
			arguments.emplace_back("--changed");
			arguments.push_back(virtualPath);
		}
	}
	return RunCommand(ResolveExecutable(), ResolveProjectDirectory(), std::move(arguments));
}

ShaderCompilerProcessResult ShaderCompilerProcess::RunToolCommand(std::string_view command) noexcept
{
	return RunCommand(ResolveExecutable(), ResolveProjectDirectory(), {std::string(command)});
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
    std::vector<std::string> arguments) noexcept
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
	        .WorkingDirectory = workingDirectory});
	result.ExitCode = process.ExitCode;
	result.Output = std::move(process.CapturedOutput);
	if (!process.FailureReason.empty())
	{
		if (!result.Output.empty())
			result.Output.push_back('\n');
		result.Output += process.FailureReason;
	}
	return result;
}
