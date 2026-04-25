#pragma once

#include <string>
#include <vector>

struct ShaderDebugArtifactSet final
{
	std::vector<std::string> CompileArguments;
	std::string CompilerOutput;
	std::string Disassembly;
	std::string ParameterMatchReportJson;
	std::string PreprocessedSource;

	bool HasAnyContent() const noexcept
	{
		return !CompileArguments.empty() || !CompilerOutput.empty() || !Disassembly.empty() || !ParameterMatchReportJson.empty() ||
		    !PreprocessedSource.empty();
	}
};