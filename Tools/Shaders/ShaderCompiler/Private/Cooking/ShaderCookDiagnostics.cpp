#include "PCH.h"

#include "Cooking/ShaderCookDiagnostics.h"

#include <format>

std::string ShaderCookDiagnostics::FormatJobContext(const ShaderCompileJob& job, std::string_view backendName, ShaderTarget target)
{
	return std::format(
	    "shader type '{}' source '{}' entry '{}' stage '{}' backend '{}' target '{}' profile '{}' compileInputHash {:016X}",
	    job.Request.ShaderTypeName,
	    job.Request.VirtualSourcePath,
	    job.Request.EntryPoint,
	    GetShaderStagePrefix(job.Request.Stage),
	    backendName,
	    GetShaderTargetName(target),
	    job.TargetProfile,
	    job.InputHash);
}
