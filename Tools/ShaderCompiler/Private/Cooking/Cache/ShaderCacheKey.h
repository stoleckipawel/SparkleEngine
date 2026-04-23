#pragma once

#include "Manifest/ShaderCookManifestTypes.h"
#include "RHI/Public/Shaders/ShaderCompileOptions.h"

#include <cstdint>
#include <string>

struct ShaderCacheKey final
{
	std::uint64_t value = 0;

	bool IsValid() const noexcept { return value != 0; }
	std::string ToHex() const;

	static ShaderCacheKey Compute(
		const ShaderCookPackageDesc& package,
		const ShaderCookStageDesc& stage,
		const ShaderCompileOptions& options,
		std::uint64_t sourceHash,
		std::uint64_t includeClosureHash,
		std::uint64_t optionsHash);
};
