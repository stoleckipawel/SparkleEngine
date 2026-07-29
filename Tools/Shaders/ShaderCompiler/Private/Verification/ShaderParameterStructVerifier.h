#pragma once

#include "ShaderReflection.h"
#include "Shaders/Authoring/ShaderParameterStruct.h"

#include <string>
#include <vector>

struct ShaderParameterStructVerificationResult final
{
	std::vector<std::string> mismatches;
	std::vector<std::string> diagnostics;

	std::string BuildJsonReport() const;
};

class ShaderParameterStructVerifier final
{
  public:
	ShaderParameterStructVerifier() = delete;

	static ShaderParameterStructVerificationResult Verify(
	    const ShaderParameterStructDescriptor& descriptor,
	    const ShaderReflection& reflection);
};
