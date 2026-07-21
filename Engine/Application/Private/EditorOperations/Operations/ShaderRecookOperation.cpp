#include "PCH.h"

#include "ShaderRecookOperation.h"

ShaderRecookExecutionResult ShaderRecookOperation::Execute(
	std::uint64_t requestId,
	std::uint64_t baselinePublicationId,
	ShaderRecookRequest request,
	std::stop_token cancellationToken)
{
	ShaderRecookExecutionResult result;
	result.RequestId = requestId;
	result.BaselinePublicationId = baselinePublicationId;
	result.Request = std::move(request);
	result.Process = ShaderCompilerProcess::RunCook(result.Request, cancellationToken);
	return result;
}
