#pragma once

#include "ShaderRecook/ShaderCompilerProcess.h"
#include "ShaderRecook/ShaderRecookRequest.h"

#include <cstdint>
#include <memory>
#include <string>

struct ShaderRecookExecutionResult final
{
	std::uint64_t RequestId = 0;
	std::uint64_t BaselinePublicationId = 0;
	ShaderRecookRequest Request;
	ShaderCompilerProcessResult Process;
};

class ShaderRecookExecutionService final
{
  public:
	ShaderRecookExecutionService();
	~ShaderRecookExecutionService();

	ShaderRecookExecutionService(const ShaderRecookExecutionService&) = delete;
	ShaderRecookExecutionService& operator=(const ShaderRecookExecutionService&) = delete;

	bool Start(
	    std::uint64_t requestId,
	    std::uint64_t baselinePublicationId,
	    ShaderRecookRequest request,
	    std::string& outErrorMessage) noexcept;
	bool TryConsume(ShaderRecookExecutionResult& outResult) noexcept;

  private:
	struct Implementation;
	std::unique_ptr<Implementation> m_implementation;
};
