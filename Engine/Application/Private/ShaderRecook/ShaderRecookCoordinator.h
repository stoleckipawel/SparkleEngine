#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "ShaderRecook/ShaderRecookPublication.h"
#include "ShaderRecook/ShaderRecookRequest.h"
#include "ShaderRecook/ShaderSourceChangeTracker.h"

class Renderer;
struct CookedShaderReloadResult;
struct ShaderRecookExecutionResult;
class ShaderRecookExecutionService;

class ShaderRecookCoordinator final
{
  public:
	using StatusHandler = std::function<void(std::string)>;
	ShaderRecookCoordinator();
	~ShaderRecookCoordinator();

	void SetStatusHandler(StatusHandler handler);
	void RequestRecook() noexcept;
	void RequestRecook(ShaderRecookRequest request) noexcept;
	void RequestReload() noexcept;
	void Update(Renderer& renderer, bool reloadRequested) noexcept;
	static std::string DescribeRequest(const ShaderRecookRequest& request);

  private:
	void StartRecook(ShaderRecookRequest request) noexcept;
	void CompleteRecook(Renderer& renderer, ShaderRecookExecutionResult result) noexcept;
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

	StatusHandler m_statusHandler;
	std::unique_ptr<ShaderRecookExecutionService> m_executionService;
	std::uint64_t m_nextRequestId = 1;
	std::uint64_t m_latestRequestId = 0;
	std::uint64_t m_lastAcceptedPublicationId = 0;
	ShaderRecookRequest m_queuedRequest;
	std::string m_lastPublicationDiagnostic;
	ShaderSourceChangeTracker m_shaderSourceChangeTracker;
	bool m_hasActiveRecook = false;
	bool m_hasQueuedRecook = false;
	bool m_reloadRequested = false;
	bool m_hasAcceptedPublication = false;
};
