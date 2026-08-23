#pragma once

#include "Cooking/ShaderCookOutput.h"

#include <cstddef>
#include <filesystem>

struct ShaderCookResult final
{
	std::filesystem::path outputDirectory;
	ShaderCookOutput output;
	std::size_t selectedShaderCount = 0;
	std::size_t compileJobCount = 0;
};
