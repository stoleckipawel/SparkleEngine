#pragma once

#include <filesystem>
#include <string>

namespace ShaderCompilerPaths
{
	std::filesystem::path CanonicalizeForCompiler(const std::filesystem::path& path);
}
