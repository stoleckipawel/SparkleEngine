#pragma once

#include "EditorOperations/Operations/ShaderRecookOperation.h"
#include "ShaderRecook/ShaderRecookRequest.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <memory>
#include <string>

class TaskExecutor;
class TaskScope;

// The sole editor-owned background operation runtime. UI code sees immutable workflow state,
// never the executor, scope, task graph, or worker completion callback.
class EditorOperationService final
{
public:
	EditorOperationService(TaskExecutor& executor, TaskScope& applicationScope);
	~EditorOperationService();

	EditorOperationService(const EditorOperationService&) = delete;
	EditorOperationService& operator=(const EditorOperationService&) = delete;

	bool StartShaderRecook(
	    std::uint64_t requestId,
	    std::uint64_t baselinePublicationId,
	    ShaderRecookRequest request,
	    std::string& outErrorMessage) noexcept;
	bool TryConsumeShaderRecook(ShaderRecookExecutionResult& outResult) noexcept;
	bool StartViewportCaptureWrite(ViewportCaptureReadback readback, std::string& outErrorMessage) noexcept;
	bool TryConsumeViewportCapture(ViewportCaptureResult& outResult) noexcept;
	void CancelDocument() noexcept;

private:
	struct ControlState;
	std::unique_ptr<ControlState> m_control;
};
