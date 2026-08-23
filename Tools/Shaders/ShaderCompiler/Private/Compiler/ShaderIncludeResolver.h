#pragma once

#include "ShaderCompileOptions.h"

#include <optional>
#include <string>

namespace ShaderIncludeResolver
{
	std::optional<std::string> ResolveIncludePath(
	    std::string_view includerPath,
	    std::string_view includePath,
	    const ShaderCompileOptions& options);

}
