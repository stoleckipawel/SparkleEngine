#pragma once

#include "ShaderCompileOptions.h"

#include <cstdint>
#include <span>
#include <vector>

class SpirVBindingNormalizer final
{
  public:
	SpirVBindingNormalizer() = delete;

	static void Normalize(
	    std::vector<std::uint8_t>& bytecode,
	    std::span<const ShaderDescriptorBindingRemap> remaps);
};
