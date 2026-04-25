#include "PCH.h"

#include "ShaderRecook/ShaderRecookCoordinator.h"

#include "Renderer.h"

#include "Core/Public/FileSystemUtils.h"

#include <array>
#include <cstdio>
#include <exception>
#include <format>
#include <sstream>
#include <system_error>
#include <utility>

void ShaderRecookCoordinator::SetStatusHandler(StatusHandler handler)
{
	m_statusHandler = std::move(handler);
}

void ShaderRecookCoordinator::RequestRecook() noexcept
{
	if (m_hasActiveRecook)
	{
		m_hasQueuedRecook = true;
		PublishStatus("Shader recook already running; queued one follow-up recook for the newest request.");
		return;
	}

	StartRecook();
}

void ShaderRecookCoordinator::Update(Renderer& renderer, bool reloadRequested) noexcept
{
	if (m_shaderSourceChangeTracker.HasChanged())
	{
		PublishStatus("Shader source change detected; scheduling out-of-process recook.");
		RequestRecook();
	}

	if (reloadRequested || HasRecookSignalChanged())
	{
		ReloadCookedShaders(renderer);
	}

	if (!m_hasActiveRecook || !m_recookFuture.valid())
	{
		return;
	}

	using namespace std::chrono_literals;
	if (m_recookFuture.wait_for(0ms) != std::future_status::ready)
	{
		return;
	}

	ProcessResult result;
	try
	{
		result = m_recookFuture.get();
	}
	catch (const std::exception& exception)
	{
		result.RequestId = m_activeRequestId;
		result.ExitCode = -1;
		result.Output = exception.what();
	}
	catch (...)
	{
		result.RequestId = m_activeRequestId;
		result.ExitCode = -1;
		result.Output = "Unknown shader recook worker failure.";
	}

	m_hasActiveRecook = false;
	m_activeRequestId = 0;
	CompleteRecook(renderer, std::move(result));

	if (m_hasQueuedRecook)
	{
		m_hasQueuedRecook = false;
		StartRecook();
	}
}

void ShaderRecookCoordinator::StartRecook() noexcept
{
	try
	{
		const std::filesystem::path shaderCompilerPath = ResolveShaderCompilerExecutable();
		if (shaderCompilerPath.empty())
		{
			PublishStatus("Shader recook failed before launch: ShaderCompiler.exe was not found in the build output.");
			return;
		}

		const std::filesystem::path projectDirectory = ResolveShowcaseProjectDirectory();
		if (projectDirectory.empty())
		{
			PublishStatus("Shader recook failed before launch: Projects/Showcase was not found from the current build output.");
			return;
		}

		const std::uint64_t requestId = m_nextRequestId++;
		m_activeRequestId = requestId;
		m_latestRequestId = requestId;
		m_hasActiveRecook = true;
		const std::filesystem::path debugArtifactDirectory = ResolveShaderDebugArtifactDirectory();
		m_recookFuture = std::async(
		    std::launch::async,
		    &ShaderRecookCoordinator::RunRecookProcess,
		    requestId,
		    shaderCompilerPath,
		    projectDirectory,
		    debugArtifactDirectory);

		PublishStatus(std::format("Shader recook #{} started via '{}'.", requestId, shaderCompilerPath.generic_string()));
	}
	catch (const std::exception& exception)
	{
		m_hasActiveRecook = false;
		m_activeRequestId = 0;
		PublishStatus(std::string("Shader recook failed before launch: ") + exception.what());
	}
	catch (...)
	{
		m_hasActiveRecook = false;
		m_activeRequestId = 0;
		PublishStatus("Shader recook failed before launch with an unknown error.");
	}
}

void ShaderRecookCoordinator::CompleteRecook(Renderer& renderer, ProcessResult result) noexcept
{
	if (result.RequestId != m_latestRequestId)
	{
		return;
	}

	if (result.ExitCode == 0)
	{
		ReloadCookedShaders(renderer);
		PublishStatus(std::format("Shader recook #{} succeeded. Reloaded cooked shader packages.\n\n{}", result.RequestId, result.Output));
		return;
	}

	PublishStatus(std::format(
	    "Shader recook #{} failed with exit code {}. Previous cooked shader packages remain active.\n\n{}",
	    result.RequestId,
	    result.ExitCode,
	    result.Output));
}

void ShaderRecookCoordinator::ReloadCookedShaders(Renderer& renderer) noexcept
{
	renderer.GetRenderHardwareInterface().WaitForIdle();
	renderer.ReloadCookedShaders();
}

void ShaderRecookCoordinator::PublishStatus(std::string status) noexcept
{
	if (!m_statusHandler)
	{
		return;
	}

	try
	{
		m_statusHandler(std::move(status));
	}
	catch (...)
	{
	}
}

bool ShaderRecookCoordinator::HasRecookSignalChanged() noexcept
{
	const std::filesystem::path signalPath = Filesystem::GetExecutableDirectory().parent_path() / "Cache" / "Shaders" / "recook.signal";
	std::error_code errorCode;
	if (!std::filesystem::exists(signalPath, errorCode) || errorCode)
	{
		return false;
	}

	const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(signalPath, errorCode);
	if (errorCode)
	{
		return false;
	}

	if (!m_hasSignalWriteTime)
	{
		m_lastSignalWriteTime = writeTime;
		m_hasSignalWriteTime = true;
		return false;
	}

	if (writeTime == m_lastSignalWriteTime)
	{
		return false;
	}

	m_lastSignalWriteTime = writeTime;
	return true;
}

std::filesystem::path ShaderRecookCoordinator::ResolveShaderCompilerExecutable() noexcept
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

std::filesystem::path ShaderRecookCoordinator::ResolveShowcaseProjectDirectory() noexcept
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

std::filesystem::path ShaderRecookCoordinator::ResolveShaderDebugArtifactDirectory() noexcept
{
	return Filesystem::GetExecutableDirectory().parent_path() / "Cache" / "Shaders" / "Debug";
}

ShaderRecookCoordinator::ProcessResult ShaderRecookCoordinator::RunRecookProcess(
    std::uint64_t requestId,
    std::filesystem::path executablePath,
    std::filesystem::path workingDirectory,
    std::filesystem::path debugArtifactDirectory) noexcept
{
	ProcessResult result;
	result.RequestId = requestId;
	result.ExecutablePath = executablePath;

	auto quote = [](const std::filesystem::path& path)
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
	};

	const std::string command = std::format(
	    "cd /d {} && {} cook --debug-artifacts {} 2>&1",
	    quote(workingDirectory),
	    quote(executablePath),
	    quote(debugArtifactDirectory));

	FILE* pipe = _popen(command.c_str(), "r");
	if (pipe == nullptr)
	{
		result.ExitCode = -1;
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