#pragma once

#include "Cooking/Cache/ShaderCacheKey.h"
#include "Manifest/ShaderCookManifestTypes.h"
#include "ShaderCompileOptions.h"

#include <cstddef>
#include <cstdint>

struct CookNode final
{
	std::size_t packageIndex = 0;
	std::size_t stageIndex = 0;
	const ShaderCookPackageDesc* package = nullptr;
	const ShaderCookStageDesc* stage = nullptr;
	ShaderCompileOptions compileOptions{};
	std::uint64_t sourceHash = 0;
	std::uint64_t includeClosureHash = 0;
	std::uint64_t optionsHash = 0;
	ShaderCacheKey cacheKey{};
};
