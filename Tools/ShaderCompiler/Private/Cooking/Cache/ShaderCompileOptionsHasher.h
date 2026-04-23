#pragma once

#include "RHI/Public/Shaders/ShaderCompileOptions.h"

#include <cstdint>

class ShaderCompileOptionsHasher final
{
  public:
	static std::uint64_t Compute(const ShaderCompileOptions& options);
};
