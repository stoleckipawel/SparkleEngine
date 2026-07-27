#include "PCH.h"

#include "Cli/CookShadersCommand.h"

#include "Analysis/CookedShaderStatsPass.h"
#include "Backend/ShaderTarget.h"
#include "Cli/CookShadersArgumentParser.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/ShaderPackageCooker.h"
#include "ToolConsole.h"

#include <iostream>
#include <ostream>
#include <string>

class CookShadersCommandExecution final
{
  public:
	static void PrintSummary(
	    const ShaderPackageCookResult& result,
	    const ShaderPackageCookSettings& settings);
	static int RunAnalysisPasses(
	    const ShaderPackageCookResult& result,
	    const ShaderPackageCookSettings& settings);

  private:
	static std::string FormatTargets(
	    std::span<const ShaderTarget> targets);
	static bool RunCookedShaderStats(
	    const ShaderPackageCookResult& result);
};

int CookShadersCommand::Run(std::span<const std::string_view> args) const
{
	if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h"))
	{
		CookShadersArgumentParser::PrintHelp(std::cout);
		return kExitCodeSuccess;
	}

	ShaderPackageCookSettings settings;
	std::string parseErrorMessage;
	if (!CookShadersArgumentParser::Parse(
	        args,
	        settings,
	        parseErrorMessage))
	{
		ToolConsole::Message(
		    std::cerr,
		    ToolConsoleSeverity::Error,
		    "Invalid shader cook arguments",
		    {ToolConsole::QuotedField("reason", parseErrorMessage)});
		return kExitCodeUsage;
	}

	ShaderPackageCooker cooker;
	const ShaderPackageCookResult cookResult = cooker.CookAll(settings);
	if (!cookResult.Succeeded())
	{
		ToolConsole::Message(
		    std::cerr,
		    ToolConsoleSeverity::Error,
		    "Failed to cook shader packages",
		    {ToolConsole::QuotedField("reason", cookResult.errorMessage)});
		return kExitCodeCookFailure;
	}

	CookShadersCommandExecution::PrintSummary(cookResult, settings);
	return CookShadersCommandExecution::RunAnalysisPasses(
	    cookResult,
	    settings);
}

void CookShadersCommandExecution::PrintSummary(
    const ShaderPackageCookResult& result,
    const ShaderPackageCookSettings& settings)
{
	ToolConsole::Summary(
	    std::cout,
	    "Cooked shader generation",
	    {ToolConsole::Field("packages", std::to_string(result.packages.size())),
	     ToolConsole::QuotedField("targets", FormatTargets(settings.targets))});
}

int CookShadersCommandExecution::RunAnalysisPasses(
    const ShaderPackageCookResult& result,
    const ShaderPackageCookSettings& settings)
{
	for (const std::string& analysisPass : settings.analysisPasses)
	{
		if (analysisPass == "cooked-shader-stats")
		{
			if (!RunCookedShaderStats(result))
			{
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

std::string CookShadersCommandExecution::FormatTargets(
    std::span<const ShaderTarget> targets)
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

bool CookShadersCommandExecution::RunCookedShaderStats(
    const ShaderPackageCookResult& result)
{
	CookedShaderStatsPassResult analysisResult;
	std::string analysisErrorMessage;
	if (!CookedShaderStatsPass::WriteCsv(
	        result.packages,
	        result.cacheDirectory / "Analysis",
	        analysisResult,
	        analysisErrorMessage))
	{
		ToolConsole::Message(
		    std::cerr,
		    ToolConsoleSeverity::Error,
		    "Failed to run analysis pass",
		    {ToolConsole::QuotedField("analysis", "cooked-shader-stats"),
		     ToolConsole::QuotedField("reason", analysisErrorMessage)});
		return false;
	}

	ToolConsole::Message(
	    std::cout,
	    ToolConsoleSeverity::Info,
	    "Analysis pass wrote output",
	    {ToolConsole::QuotedField("analysis", "cooked-shader-stats"),
	     ToolConsole::Field("rows", std::to_string(analysisResult.rowCount)),
	     ToolConsole::PathField("output", analysisResult.outputPath)});
	return true;
}
