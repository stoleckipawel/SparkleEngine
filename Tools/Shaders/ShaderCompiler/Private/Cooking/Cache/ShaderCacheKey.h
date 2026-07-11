#pragma once

#include "Cooking/ShaderCookTypes.h"

#include <cstdint>
#include <string>
#include <string_view>

struct ShaderCacheKey final
{
	std::uint64_t value = 0;

	bool IsValid() const noexcept { return value != 0; }
	std::string ToHex() const;

	static ShaderCacheKey Compute(
		const ShaderCookPackageDesc& package,
		std::uint64_t sourceHash,
		std::uint64_t includeClosureHash,
		std::uint64_t optionsHash,
		std::string_view backendName,
		std::uint64_t backendVersion);
};
