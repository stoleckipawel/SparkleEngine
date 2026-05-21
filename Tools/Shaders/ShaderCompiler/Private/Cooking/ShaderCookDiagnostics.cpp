#include "PCH.h"

#include "Cooking/ShaderCookDiagnostics.h"

#include <format>

std::string ShaderCookDiagnostics::FormatNodeContext(
    const CookNode& node,
    std::string_view backendName,
    ShaderTarget target)
{
	return std::format(
	    "shader package '{}' shader '{}' stage '{}' backend '{}' target '{}'",
	    node.package->packageId,
	    node.stage->sourcePath.generic_string(),
	    GetShaderStagePrefix(node.stage->stage),
	    backendName,
	    GetShaderTargetName(target));
}