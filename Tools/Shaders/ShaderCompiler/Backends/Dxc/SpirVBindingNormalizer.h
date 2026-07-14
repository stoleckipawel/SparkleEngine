#pragma once

#include "ShaderCompileOptions.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

class SpirVBindingNormalizer final
{
  public:
	SpirVBindingNormalizer() = delete;

	static bool Normalize(
	    std::vector<std::uint8_t>& bytecode,
	    std::span<const ShaderDescriptorBindingRemap> remaps,
	    std::string& outErrorMessage);
};
