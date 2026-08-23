#pragma once

#include "Compiler/ShaderCompileRequest.h"

#include <cstdint>

class ShaderCompileRequestHasher final
{
public:
	ShaderCompileRequestHasher() = delete;

	static std::uint64_t Compute(const ShaderCompileRequest& request);
};
