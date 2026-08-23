#include "PCH.h"

#include "Compiler/ShaderIncludeResolver.h"

#include "Compiler/ShaderSourceMountTable.h"
#include "Core/Public/Diagnostics/Error.h"

namespace ShaderIncludeResolver
{
	std::optional<std::string> ResolveIncludePath(
	    std::string_view includerPath,
	    std::string_view includePath,
	    const ShaderCompileOptions& options)
	{
		if (includePath.empty())
		{
			return std::nullopt;
		}

		if (options.SourceMounts == nullptr)
		{
			return std::nullopt;
		}
		try
		{
			return options.SourceMounts->ResolveInclude(includerPath, includePath);
		}
		catch (const Diagnostics::Error&)
		{
			return std::nullopt;
		}
	}
}
