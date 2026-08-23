#include "PCH.h"

#include "Cooking/ShaderCookDiagnostics.h"

#include <format>

std::string ShaderCookDiagnostics::FormatNodeContext(
    const CookNode& node,
    std::string_view backendName,
    ShaderTarget target)
{
	return std::format(
	    "shader package '{}' shader '{}' entry '{}' stage '{}' backend '{}' target '{}' profile '{}' compileInputHash {:016X}",
	    node.jobIdentity.packageId,
	    node.jobIdentity.sourcePath,
	    node.jobIdentity.entryPoint,
	    GetShaderStagePrefix(node.jobIdentity.stage),
	    backendName,
	    GetShaderTargetName(target),
	    node.jobIdentity.profileName,
	    node.jobIdentity.compileInputHash);
}
