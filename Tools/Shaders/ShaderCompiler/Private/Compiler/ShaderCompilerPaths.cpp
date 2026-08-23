#include "PCH.h"

#include "Compiler/ShaderCompilerPaths.h"

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
}
