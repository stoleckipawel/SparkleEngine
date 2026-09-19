#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "EditorOperations/EditorOperationSlot.h"
#include "ShaderRecook/ShaderCompilerProcess.h"
#include "ShaderRecook/ShaderRecookPublication.h"
#include "ShaderRecook/ShaderRecookRequest.h"
#include "ShaderRecook/ShaderSourceChangeTracker.h"

class Renderer;
class EditorOperationRuntime;

class ShaderRecookCoordinator final
{
public:
	using StatusHandler = std::function<void(std::string)>;
	explicit ShaderRecookCoordinator(EditorOperationRuntime& operations);
	~ShaderRecookCoordinator();

	void SetStatusHandler(StatusHandler handler);
	void RequestRecook() noexcept;
	void RequestRecook(ShaderRecookRequest request) noexcept;
	void RequestReload() noexcept;
	void Update(Renderer& renderer, bool reloadRequested) noexcept;
	static std::string DescribeRequest(const ShaderRecookRequest& request);

private:
	struct ExecutionResult final
	{
		std::uint64_t RequestId = 0;
		std::uint64_t BaselinePublicationId = 0;
		ShaderRecookRequest Request;
		ShaderCompilerProcessResult Process;
	};

	void StartRecook(ShaderRecookRequest request) noexcept;
	void CompleteRecook(Renderer& renderer, ExecutionResult result) noexcept;
	void ReloadShaders(Renderer& renderer);
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
	EditorOperationSlot<ExecutionResult> m_operation;
	std::uint64_t m_nextRequestId = 1;
	std::uint64_t m_latestRequestId = 0;
	std::uint64_t m_lastAcceptedPublicationId = 0;
	ShaderRecookRequest m_queuedRequest;
	std::string m_lastPublicationDiagnostic;
	ShaderSourceChangeTracker m_shaderSourceChangeTracker;
	bool m_hasQueuedRecook = false;
	bool m_reloadRequested = false;
	bool m_hasAcceptedPublication = false;
};
