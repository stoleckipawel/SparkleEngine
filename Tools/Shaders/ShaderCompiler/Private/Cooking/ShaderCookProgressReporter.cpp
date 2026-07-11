#include "PCH.h"

#include "Cooking/ShaderCookProgressReporter.h"

#include "Cooking/ShaderCookSettings.h"
#include "ToolConsole.h"

#include <algorithm>
#include <iostream>

void ShaderCookProgressReporter::PrintPlanSummary(
    const ShaderCookPipelinePlan& plan,
    const ShaderPackageCookSettings& settings)
{
	ToolConsole::Summary(
	    std::cout,
	    "ShaderCompiler cook plan",
	    {ToolConsole::Field("packages", std::to_string(plan.packages.size())),
	     ToolConsole::Field("stageJobs", std::to_string(plan.nodes.size())),
	     ToolConsole::QuotedField("backend", settings.backendName),
	     ToolConsole::Field("cache", settings.useCache ? "enabled" : "disabled")});
}

void ShaderCookProgressReporter::PrintPackageProgress(
    const ShaderCookPipelinePlan& plan,
    const CookNode& node)
{
	const ShaderCookPackageDesc& package = plan.packages[node.packageIndex];
	const std::size_t packageJobCount = std::count_if(
	    plan.nodes.begin(),
	    plan.nodes.end(),
	    [packageIndex = node.packageIndex](const CookNode& candidate)
	    {
		    return candidate.packageIndex == packageIndex;
	    });
	ToolConsole::Progress(
	    std::cout,
	    "Cooking",
	    "shader-package",
	    node.packageIndex + 1u,
	    plan.packages.size(),
	    package.packageId,
	    {ToolConsole::Field("jobs", std::to_string(packageJobCount))});
}

void ShaderCookProgressReporter::PrintStageProgress(
    const ShaderCookPipelinePlan& plan,
    const ShaderCookExecutionCounters& counters,
    const CookNode& node,
    std::string_view backendName,
    std::string_view status)
{
	ToolConsole::Progress(
	    std::cout,
	    "Processing",
	    "shader-stage",
	    counters.processedNodeCount + 1u,
	    plan.nodes.size(),
	    node.package->packageId,
	    {ToolConsole::Field("status", std::string(status)),
	     ToolConsole::Field("stage", GetShaderStagePrefix(node.jobIdentity.stage)),
	     ToolConsole::Field("target", node.jobIdentity.targetName),
	     ToolConsole::QuotedField("backend", std::string(backendName)),
	     ToolConsole::QuotedField("source", node.jobIdentity.sourcePath.generic_string()),
	     ToolConsole::Field("jobKey", node.cacheKey.ToHex())});
}
