#include "PCH.h"

#include "ShaderRecook/ShaderRecookCoordinator.h"

#include "Renderer.h"

#include <exception>
#include <format>
#include <system_error>
#include <utility>

void ShaderRecookCoordinator::SetStatusHandler(StatusHandler handler)
{
	m_statusHandler = std::move(handler);
}

void ShaderRecookCoordinator::RequestRecook() noexcept
{
	RequestRecook(ShaderRecookRequest{.Type = ShaderRecookRequestType::Global});
}

void ShaderRecookCoordinator::RequestRecook(ShaderRecookRequest request) noexcept
{
	if (m_hasActiveRecook)
	{
		m_queuedRequest = std::move(request);
		m_hasQueuedRecook = true;
		PublishStatus(std::format(
		    "Shader recook already running; queued one follow-up request for {}.",
		    DescribeRequest(m_queuedRequest)));
		return;
	}

	StartRecook(std::move(request));
}

void ShaderRecookCoordinator::RequestReload() noexcept
{
	m_reloadRequested = true;
	PublishStatus("Shader reload requested; cooked shader packages will reload at the next coordinator update.");
}

void ShaderRecookCoordinator::Update(Renderer& renderer, bool reloadRequested) noexcept
{
	if (m_shaderSourceChangeTracker.HasChanged())
	{
		PublishStatus("Shader source change detected; scheduling out-of-process recook.");
		RequestRecook(ShaderRecookRequest{.Type = ShaderRecookRequestType::Changed});
	}

	if (m_reloadRequested || reloadRequested || HasRecookSignalChanged())
	{
		m_reloadRequested = false;
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
		result.Request = m_activeRequest;
		result.Process.Output = exception.what();
	}
	catch (...)
	{
		result.RequestId = m_activeRequestId;
		result.Request = m_activeRequest;
		result.Process.Output = "Unknown shader recook worker failure.";
	}

	m_hasActiveRecook = false;
	m_activeRequestId = 0;
	m_activeRequest = {};
	CompleteRecook(renderer, std::move(result));

	if (m_hasQueuedRecook)
	{
		ShaderRecookRequest queuedRequest = std::move(m_queuedRequest);
		m_queuedRequest = {};
		m_hasQueuedRecook = false;
		StartRecook(std::move(queuedRequest));
	}
}

void ShaderRecookCoordinator::StartRecook(ShaderRecookRequest request) noexcept
{
	try
	{
		const std::uint64_t requestId = m_nextRequestId++;
		m_activeRequestId = requestId;
		m_latestRequestId = requestId;
		m_activeRequest = request;
		m_hasActiveRecook = true;
		m_recookFuture = std::async(
		    std::launch::async,
		    &ShaderRecookCoordinator::RunRecookProcess,
		    requestId,
		    request);

		PublishStatus(std::format(
		    "Shader recook #{} started for {} via the shader compiler process seam.",
		    requestId,
		    DescribeRequest(request)));
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

	if (result.Process.Succeeded())
	{
		ReloadCookedShaders(renderer);
		PublishStatus(std::format(
		    "Shader recook #{} ({}) succeeded. Reloaded cooked shader packages.\nCommand: {}\n\n{}",
		    result.RequestId,
		    DescribeRequest(result.Request),
		    result.Process.CommandLine,
		    result.Process.Output));
		return;
	}

	PublishStatus(std::format(
	    "Shader recook #{} ({}) failed with exit code {}. Previous cooked shader packages remain active; no recook signal was accepted and old artifacts remain loaded.\nCommand: {}\n\n{}",
	    result.RequestId,
	    DescribeRequest(result.Request),
	    result.Process.ExitCode,
	    result.Process.CommandLine,
	    result.Process.Output));
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

std::string ShaderRecookCoordinator::DescribeRequest(const ShaderRecookRequest& request)
{
	switch (request.Type)
	{
		case ShaderRecookRequestType::Changed:
			return "changed shader sources";
		case ShaderRecookRequestType::ShaderPathOrId:
			return request.Target.empty() ? "targeted shader <empty>" : "targeted shader '" + request.Target + "'";
		case ShaderRecookRequestType::Global:
		default:
			return "all global shaders";
	}
}

bool ShaderRecookCoordinator::HasRecookSignalChanged() noexcept
{
	const std::filesystem::path signalPath = ShaderCompilerProcess::ResolveRecookSignalPath();
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

ShaderRecookCoordinator::ProcessResult ShaderRecookCoordinator::RunRecookProcess(
    std::uint64_t requestId,
	ShaderRecookRequest request) noexcept
{
	ProcessResult result;
	result.RequestId = requestId;
	result.Request = request;
	result.Process = ShaderCompilerProcess::RunCook(request);
	return result;
}