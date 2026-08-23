#include "PCH.h"

#include "Cli/CookShadersCommand.h"

#include "Analysis/CookedShaderStatsPass.h"
#include "RHI/Public/Shaders/ShaderTarget.h"
#include "Cli/CookShadersArgumentParser.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/GlobalShaderCooker.h"
#include "Core/Public/Diagnostics/Error.h"
#include "ToolConsole.h"

#include <iostream>
#include <ostream>
#include <string>

class CookShadersCommandExecution final
{
public:
	static void PrintSummary(const ShaderCookResult& result, const ShaderCookSettings& settings);
	static int RunAnalysisPasses(const ShaderCookResult& result, const ShaderCookSettings& settings);

private:
	static std::string FormatTargets(std::span<const ShaderTarget> targets);
	static void RunCookedShaderStats(const ShaderCookResult& result);
};

int CookShadersCommand::Run(std::span<const std::string_view> args) const
{
	if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h"))
	{
		CookShadersArgumentParser::PrintHelp(std::cout);
		return kExitCodeSuccess;
	}

	ShaderCookSettings settings;
	try
	{
		settings = CookShadersArgumentParser::Parse(args);
	}
	catch (const Diagnostics::Error& error)
	{
		ToolConsole::Message(
		    std::cerr,
		    ToolConsoleSeverity::Error,
		    "Invalid shader cook arguments",
		    {ToolConsole::QuotedField("reason", error.what())});
		return kExitCodeUsage;
	}

	GlobalShaderCooker cooker;
	ShaderCookResult cookResult;
	try
	{
		cookResult = cooker.CookAll(settings);
	}
	catch (const Diagnostics::Error& error)
	{
		ToolConsole::Message(
		    std::cerr,
		    ToolConsoleSeverity::Error,
		    "Failed to publish cooked shaders",
		    {ToolConsole::QuotedField("reason", error.what())});
		return kExitCodeCookFailure;
	}

	CookShadersCommandExecution::PrintSummary(cookResult, settings);
	if (cookResult.output.entries.empty())
	{
		return kExitCodeNoWork;
	}
	return CookShadersCommandExecution::RunAnalysisPasses(cookResult, settings);
}

void CookShadersCommandExecution::PrintSummary(const ShaderCookResult& result, const ShaderCookSettings& settings)
{
	ToolConsole::Summary(
	    std::cout,
	    "Cooked shader generation",
	    {ToolConsole::Field("shaderTypes", std::to_string(result.selectedShaderCount)),
	        ToolConsole::Field("compileJobs", std::to_string(result.compileJobCount)),
	        ToolConsole::Field("mapEntries", std::to_string(result.output.entries.size())),
	        ToolConsole::Field("uniqueCodeRecords", std::to_string(result.output.uniqueCodeCount)),
	        ToolConsole::QuotedField("targets", FormatTargets(settings.targets))});
}

int CookShadersCommandExecution::RunAnalysisPasses(const ShaderCookResult& result, const ShaderCookSettings& settings)
{
	for (const std::string& analysisPass : settings.analysisPasses)
	{
		if (analysisPass == "cooked-shader-stats")
		{
			try
			{
				RunCookedShaderStats(result);
			}
			catch (const Diagnostics::Error& error)
			{
				ToolConsole::Message(
				    std::cerr,
				    ToolConsoleSeverity::Error,
				    "Failed to run analysis pass",
				    {ToolConsole::QuotedField("analysis", "cooked-shader-stats"), ToolConsole::QuotedField("reason", error.what())});
				return kExitCodeCookFailure;
			}
			continue;
		}

		ToolConsole::Message(
		    std::cerr,
		    ToolConsoleSeverity::Error,
		    "Unknown analysis pass",
		    {ToolConsole::QuotedField("analysis", analysisPass)});
		return kExitCodeUsage;
	}

	return kExitCodeSuccess;
}

std::string CookShadersCommandExecution::FormatTargets(std::span<const ShaderTarget> targets)
{
	std::string result;
	for (std::size_t index = 0; index < targets.size(); ++index)
	{
		if (index > 0)
		{
			result += ',';
		}

		result += GetShaderTargetName(targets[index]);
	}

	return result;
}

void CookShadersCommandExecution::RunCookedShaderStats(const ShaderCookResult& result)
{
	const CookedShaderStatsReport report = CookedShaderStatsPass::WriteCsv(result.output, result.outputDirectory / "Analysis");

	ToolConsole::Message(
	    std::cout,
	    ToolConsoleSeverity::Info,
	    "Analysis pass wrote output",
	    {ToolConsole::QuotedField("analysis", "cooked-shader-stats"),
	        ToolConsole::Field("rows", std::to_string(report.rowCount)),
	        ToolConsole::PathField("output", report.outputPath)});
}
