#include "PCH.h"

#include "Compiler/ShaderIncludeResolver.h"

#include "Compiler/ShaderCompilerPaths.h"
#include "Core/Public/Paths/PathUtils.h"

namespace ShaderIncludeResolver
{
	std::optional<std::filesystem::path> ResolveIncludePath(
	    const std::filesystem::path& includerPath,
	    std::string_view includePath,
	    const ShaderCompileOptions& options)
	{
		if (includePath.empty())
		{
			return std::nullopt;
		}

		std::error_code errorCode;
		const std::filesystem::path includeRelativePath(includePath);
		if (includeRelativePath.is_absolute())
		{
			const std::filesystem::path absolutePath = ShaderCompilerPaths::CanonicalizeForCompiler(includeRelativePath);
			if (std::filesystem::exists(absolutePath, errorCode) && !errorCode)
			{
				return absolutePath;
			}
			return std::nullopt;
		}

		auto checkRoot = [&](const std::filesystem::path& root) -> std::optional<std::filesystem::path>
		{
			if (root.empty())
			{
				return std::nullopt;
			}

			errorCode.clear();
			const std::filesystem::path candidate = ShaderCompilerPaths::CanonicalizeForCompiler(root / includeRelativePath);
			if (std::filesystem::exists(candidate, errorCode) && !errorCode)
			{
				return candidate;
			}
			return std::nullopt;
		};

		if (const auto fromLocal = checkRoot(includerPath.parent_path()))
		{
			return fromLocal;
		}

		if (const auto fromPrimary = checkRoot(options.IncludeDir))
		{
			return fromPrimary;
		}

		for (const std::filesystem::path& includeRoot : options.AdditionalIncludeDirs)
		{
			if (const auto fromAdditional = checkRoot(includeRoot))
			{
				return fromAdditional;
			}
		}

		return std::nullopt;
	}

	std::wstring MakeResolvedPathKey(const std::filesystem::path& path)
	{
		return Paths::MakePathKey(ShaderCompilerPaths::CanonicalizeForCompiler(path));
	}
}
