#include "PCH.h"

#include "Compiler/ShaderIncludeResolver.h"

#include "Compiler/ShaderSourceMountTable.h"
#include "Core/Public/Diagnostics/Error.h"

namespace ShaderIncludeResolver
{
	std::optional<std::string> ResolveIncludePath(
	    std::string_view includerPath,
	    std::string_view includePath,
	    const ShaderCompileRequest& request)
	{
		if (includePath.empty())
		{
			return std::nullopt;
		}

		try
		{
			return request.SourceMounts.get().ResolveInclude(includerPath, includePath);
		}
		catch (const Diagnostics::Error&)
		{
			return std::nullopt;
		}
	}
}
