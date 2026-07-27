#pragma once

#include <filesystem>
#include <string>

namespace ShaderCompilerPaths
{
	std::filesystem::path CanonicalizeForCompiler(const std::filesystem::path& path);
	std::string MakePathArgument(const std::filesystem::path& path);
	std::string MakeIncludeDirectoryArgument(const std::filesystem::path& path);
	std::wstring MakeWidePathArgument(const std::filesystem::path& path);
	std::wstring MakeWideIncludeDirectoryArgument(const std::filesystem::path& path);
}
