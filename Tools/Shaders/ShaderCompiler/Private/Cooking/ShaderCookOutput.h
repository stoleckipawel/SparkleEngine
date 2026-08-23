#pragma once

#include "RHI/Public/Shaders/ShaderMap.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct ShaderCookedEntry final
{
	ShaderTypeId shaderType = 0;
	std::string shaderName;
	ShaderTarget target = kDefaultShaderTarget;
	ShaderCodeHash codeHash = 0;
	std::size_t codeSizeInBytes = 0;
};

struct ShaderCookOutput final
{
	std::filesystem::path mapPath;
	std::filesystem::path libraryPath;
	std::vector<ShaderCookedEntry> entries;
	std::size_t uniqueCodeCount = 0;
};
