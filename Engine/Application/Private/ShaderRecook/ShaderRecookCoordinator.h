#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <future>
#include <string>

#include "ShaderRecook/ShaderCompilerProcess.h"
#include "ShaderRecook/ShaderRecookRequest.h"
#include "ShaderRecook/ShaderSourceChangeTracker.h"

class Renderer;

class ShaderRecookCoordinator final
{
  public:
	using StatusHandler = std::function<void(std::string)>;

	void SetStatusHandler(StatusHandler handler);
	void RequestRecook() noexcept;
	void RequestRecook(ShaderRecookRequest request) noexcept;
	void RequestReload() noexcept;
	void Update(Renderer& renderer, bool reloadRequested) noexcept;
	static std::string DescribeRequest(const ShaderRecookRequest& request);

  private:
	struct ProcessResult final
	{
		std::uint64_t RequestId = 0;
		ShaderRecookRequest Request;
		ShaderCompilerProcessResult Process;
	};

	void StartRecook(ShaderRecookRequest request) noexcept;
	void CompleteRecook(Renderer& renderer, ProcessResult result) noexcept;
	void ReloadCookedShaders(Renderer& renderer) noexcept;
	void PublishStatus(std::string status) noexcept;
	bool HasRecookSignalChanged() noexcept;

	static ProcessResult RunRecookProcess(std::uint64_t requestId, ShaderRecookRequest request) noexcept;

	StatusHandler m_statusHandler;
	std::future<ProcessResult> m_recookFuture;
	std::uint64_t m_nextRequestId = 1;
	std::uint64_t m_activeRequestId = 0;
	std::uint64_t m_latestRequestId = 0;
	ShaderRecookRequest m_activeRequest;
	ShaderRecookRequest m_queuedRequest;
	std::filesystem::file_time_type m_lastSignalWriteTime{};
	ShaderSourceChangeTracker m_shaderSourceChangeTracker;
	bool m_hasActiveRecook = false;
	bool m_hasQueuedRecook = false;
	bool m_reloadRequested = false;
	bool m_hasSignalWriteTime = false;
};