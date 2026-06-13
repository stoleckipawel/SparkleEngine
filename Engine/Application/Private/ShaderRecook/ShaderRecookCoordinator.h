#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <string>

#include "ShaderRecook/ShaderCompilerProcess.h"
#include "ShaderRecook/ShaderRecookPublication.h"
#include "ShaderRecook/ShaderRecookRequest.h"
#include "ShaderRecook/ShaderSourceChangeTracker.h"

class Renderer;
struct CookedShaderReloadResult;

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
		std::uint64_t BaselinePublicationId = 0;
		ShaderRecookRequest Request;
		ShaderCompilerProcessResult Process;
	};

	void StartRecook(ShaderRecookRequest request) noexcept;
	void CompleteRecook(Renderer& renderer, ProcessResult result) noexcept;
	CookedShaderReloadResult ReloadCookedShaders(Renderer& renderer) noexcept;
	void HandleManualReload(Renderer& renderer) noexcept;
	void HandleExternalRecookPublication(Renderer& renderer) noexcept;
	void PublishStatus(std::string status) noexcept;
	std::uint64_t ReadCurrentPublicationId() noexcept;
	ShaderRecookPublicationReadResult ReadRecookPublication() noexcept;
	bool TryAcceptFreshPublication(
	    const ShaderRecookPublicationReadResult& readResult,
	    std::uint64_t minimumPublicationId,
	    ShaderRecookPublication& outPublication,
	    std::string& outDiagnostic) noexcept;

	static ProcessResult RunRecookProcess(
	    std::uint64_t requestId,
	    std::uint64_t baselinePublicationId,
	    ShaderRecookRequest request) noexcept;

	StatusHandler m_statusHandler;
	std::future<ProcessResult> m_recookFuture;
	std::uint64_t m_nextRequestId = 1;
	std::uint64_t m_activeRequestId = 0;
	std::uint64_t m_latestRequestId = 0;
	std::uint64_t m_activeBaselinePublicationId = 0;
	std::uint64_t m_lastAcceptedPublicationId = 0;
	ShaderRecookRequest m_activeRequest;
	ShaderRecookRequest m_queuedRequest;
	std::string m_lastPublicationDiagnostic;
	ShaderSourceChangeTracker m_shaderSourceChangeTracker;
	bool m_hasActiveRecook = false;
	bool m_hasQueuedRecook = false;
	bool m_reloadRequested = false;
	bool m_hasAcceptedPublication = false;
};
