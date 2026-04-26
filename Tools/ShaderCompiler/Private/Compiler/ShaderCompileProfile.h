#pragma once

#include "ShaderCompileOptions.h"

#include <string>

class ShaderCompileProfile final
{
  public:
	ShaderCompileProfile() = delete;

	static std::string BuildTargetProfile(const ShaderCompileOptions& options);
};