#pragma once

#include "Cooking/ShaderCompileJob.h"

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class ShaderCompileFailureReplay final
{
public:
	ShaderCompileFailureReplay() = delete;

	static void Write(const std::filesystem::path& cookedShaderRoot, const ShaderCompileJob& job, std::string_view diagnostic) noexcept;

private:
	static std::string BoundText(std::string_view text, std::size_t maximumBytes);
	static std::vector<std::string> BoundStrings(
	    std::span<const std::string> values,
	    std::size_t maximumCount,
	    std::size_t maximumBytesPerValue);
};
