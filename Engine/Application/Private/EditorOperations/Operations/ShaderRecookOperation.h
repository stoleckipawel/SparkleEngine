#pragma once

#include "ShaderRecook/ShaderCompilerProcess.h"
#include "ShaderRecook/ShaderRecookRequest.h"

#include <cstdint>
#include <stop_token>

struct ShaderRecookExecutionResult final
{
	std::uint64_t RequestId = 0;
	std::uint64_t BaselinePublicationId = 0;
	ShaderRecookRequest Request;
	ShaderCompilerProcessResult Process;
};

// Operation-specific capability. It knows how to invoke and interpret the shader compiler,
// but owns no task scope, publication slot, or editor workflow state.
class ShaderRecookOperation final
{
  public:
	static ShaderRecookExecutionResult Execute(
	    std::uint64_t requestId,
	    std::uint64_t baselinePublicationId,
	    ShaderRecookRequest request,
	    std::stop_token cancellationToken);
};
