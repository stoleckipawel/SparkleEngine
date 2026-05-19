#include "PCH.h"

#include "Compiler/ShaderCompilerPathUtils.h"

#include "Core/Public/Strings/StringUtils.h"

namespace ShaderCompilerPaths
{
	std::filesystem::path CanonicalizeForCompiler(const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return {};
		}

		std::error_code errorCode;
		const std::filesystem::path absolutePath = std::filesystem::absolute(path, errorCode);
		const std::filesystem::path normalizedPath = (errorCode ? path : absolutePath).lexically_normal();

		if (const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(normalizedPath, errorCode); !errorCode)
		{
			return canonicalPath.lexically_normal();
		}

		return normalizedPath;
	}

	std::string MakePathArgument(const std::filesystem::path& path)
	{
		return CanonicalizeForCompiler(path).generic_string();
	}

	std::string MakeIncludeDirectoryArgument(const std::filesystem::path& path)
	{
		std::string genericPath = MakePathArgument(path);
		if (!genericPath.empty() && genericPath.back() != '/')
		{
			genericPath.push_back('/');
		}
		return genericPath;
	}

	std::wstring MakeWidePathArgument(const std::filesystem::path& path)
	{
		const std::string genericPath = MakePathArgument(path);
		return Strings::ToWide(std::string_view{genericPath});
	}

	std::wstring MakeWideIncludeDirectoryArgument(const std::filesystem::path& path)
	{
		const std::string genericPath = MakeIncludeDirectoryArgument(path);
		return Strings::ToWide(std::string_view{genericPath});
	}
}