#pragma once

#include "Compiler/ShaderCompileRequest.h"

#include <string_view>
#include <optional>
#include <string>

namespace ShaderIncludeResolver
{
	std::optional<std::string> ResolveIncludePath(
	    std::string_view includerPath,
	    std::string_view includePath,
	    const ShaderCompileRequest& request);

}
