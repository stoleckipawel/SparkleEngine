#pragma once

#include "ShaderCompileOptions.h"

#include <string>

class ShaderCompileProfile final
{
  public:
	ShaderCompileProfile() = delete;

	static const char* GetShaderModelProfileName(ShaderTarget target);
	static const char* GetSpirVProfileName(ShaderTarget target);
	static const char* GetSlangTargetProfileName(ShaderTarget target);
	static std::string BuildTargetProfile(const ShaderCompileOptions& options);
};