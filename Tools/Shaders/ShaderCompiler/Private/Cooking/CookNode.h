#pragma once

#include "Cooking/Cache/ShaderCacheKey.h"
#include "Cooking/ShaderCookTypes.h"
#include "ShaderCompileOptions.h"
#include "Shaders/Authoring/ShaderParameterStruct.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

struct CookNode final
{
	std::size_t packageIndex = 0;
	std::size_t stageIndex = 0;
	const ShaderCookPackageDesc* package = nullptr;
	const ShaderCookStageDesc* stage = nullptr;
	std::string backendName;
	ShaderCompileOptions compileOptions{};
	std::optional<ShaderParameterStructDescriptor> parameterStructDescriptor;
	std::uint64_t sourceHash = 0;
	std::uint64_t includeClosureHash = 0;
	std::uint64_t optionsHash = 0;
	ShaderCacheKey cacheKey{};
};