#pragma once

#include "ShaderCompileOptions.h"

#include <filesystem>
#include <optional>
#include <string>

namespace ShaderIncludeResolver
{
	std::optional<std::filesystem::path> ResolveIncludePath(
	    const std::filesystem::path& includerPath,
	    std::string_view includePath,
	    const ShaderCompileOptions& options);

	std::wstring MakeResolvedPathKey(const std::filesystem::path& path);
}