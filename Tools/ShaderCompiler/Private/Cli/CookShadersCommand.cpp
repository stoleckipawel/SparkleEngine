#include "PCH.h"

#include "Cli/CookShadersCommand.h"

#include "Analysis/PsoStatsPass.h"
#include "Backend/ShaderTarget.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/ShaderPackageCooker.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"
#include "ToolConsole.h"

#include <iostream>
#include <string>
#include <vector>

namespace
{
	bool ContainsTarget(std::span<const ShaderTarget> targets, ShaderTarget target) noexcept
	{
		for (const ShaderTarget existingTarget : targets)
		{
			if (existingTarget == target)
			{
				return true;
			}
		}

		return false;
	}

	std::string FormatTargets(std::span<const ShaderTarget> targets)
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

	void AppendAnalysisPasses(std::string_view value, std::vector<std::string>& outPasses)
	{
		for (const std::string_view token : Strings::Split(value, ',', false))
		{
			outPasses.emplace_back(token);
		}
	}
}

bool CookShadersCommand::TryParseArguments(
	std::span<const std::string_view> args,
	ShaderPackageCookSettings& outSettings,
	std::string& outErrorMessage)
{
	bool targetWasSpecified = false;
	for (std::size_t index = 0; index < args.size(); ++index)
	{
		if (args[index] == "--no-cache")
		{
			outSettings.useCache = false;
			continue;
		}

		if (args[index] == "--cache-dir")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --cache-dir";
				return false;
			}

			outSettings.cacheDirectory = std::filesystem::path(std::string(args[index + 1]));
			++index;
			continue;
		}

		if (args[index] == "--shader")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --shader";
				return false;
			}

			outSettings.singleShaderPath = std::filesystem::path(std::string(args[index + 1]));
			++index;
			continue;
		}

		if (args[index] == "--target")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --target";
				return false;
			}

			const std::string_view value = args[index + 1];
			ShaderTarget parsed = kDefaultShaderTarget;
			bool matched = false;
			for (std::uint16_t candidate = static_cast<std::uint16_t>(ShaderTarget::DxilSm60);
			     candidate <= static_cast<std::uint16_t>(ShaderTarget::SpirV16);
			     ++candidate)
			{
				const auto target = static_cast<ShaderTarget>(candidate);
				if (value == GetShaderTargetName(target))
				{
					parsed = target;
					matched = true;
					break;
				}
			}

			if (!matched)
			{
				outErrorMessage = "Unknown shader target '" + std::string(value) + "'";
				return false;
			}

			if (!targetWasSpecified)
			{
				outSettings.targets.clear();
				targetWasSpecified = true;
			}
			if (!ContainsTarget(outSettings.targets, parsed))
			{
				outSettings.targets.push_back(parsed);
			}
			++index;
			continue;
		}

		if (args[index] == "--backend")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --backend";
				return false;
			}

			outSettings.backendName = std::string(args[index + 1]);
			++index;
			continue;
		}

		if (args[index] == "--debug-artifacts")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --debug-artifacts";
				return false;
			}

			outSettings.debugArtifactDirectory = std::filesystem::path(std::string(args[index + 1]));
			++index;
			continue;
		}

		if (args[index] == "--analysis")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --analysis";
				return false;
			}

			AppendAnalysisPasses(args[index + 1], outSettings.analysisPasses);
			++index;
			continue;
		}

		if (args[index] == "--verification-self-test")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --verification-self-test";
				return false;
			}

			if (args[index + 1] == "parameter-mismatch")
			{
				outSettings.forceParameterStructMismatchForValidation = true;
				++index;
				continue;
			}

			if (args[index + 1] == "missing-include")
			{
				outSettings.forceMissingIncludeForValidation = true;
				++index;
				continue;
			}

			outErrorMessage = "Unknown verification self-test '" + std::string(args[index + 1]) + "'";
			return false;
		}

		outErrorMessage = "Unknown cook argument '" + std::string(args[index]) + "'";
		return false;
	}

	outErrorMessage.clear();
	return true;
}

int CookShadersCommand::Run(std::span<const std::string_view> args) const
{
	ShaderPackageCookSettings settings;
	std::string parseErrorMessage;
	if (!TryParseArguments(args, settings, parseErrorMessage))
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

	ToolConsole::Summary(
	    std::cout,
	    "ShaderCompiler summary",
	    {ToolConsole::Field("packages", std::to_string(cookResult.packages.size())),
	     ToolConsole::PathField("packageRoot", Paths::CookedShaderPackageRoot()),
	     ToolConsole::PathField("registry", cookResult.registryPath),
	     ToolConsole::QuotedField("targets", FormatTargets(settings.targets)),
	     ToolConsole::PathField("recookSignal", cookResult.recookSignalPath),
	     ToolConsole::Field("backendInvocations", std::to_string(cookResult.backendInvocationCount)),
	     ToolConsole::Field("cacheHits", std::to_string(cookResult.cacheHitCount)),
	     ToolConsole::Field("cacheMisses", std::to_string(cookResult.cacheMissCount)),
	     ToolConsole::PathField("cacheDir", cookResult.cacheDirectory)});

	ToolConsole::ListHeader(std::cout, "Cooked shader packages");
	for (std::size_t packageIndex = 0; packageIndex < cookResult.packages.size(); ++packageIndex)
	{
		const CookedShaderPackageOutput& package = cookResult.packages[packageIndex];
		ToolConsole::ListItem(
		    std::cout,
		    packageIndex + 1u,
		    {ToolConsole::QuotedField("name", package.packageId),
		     ToolConsole::QuotedField("bindingLayout", package.bindingLayoutId),
		     ToolConsole::Field("key", Formatting::FormatHexUInt64(package.packageKey)),
		     ToolConsole::PathField("output", package.outputPath)});
	}

	for (const std::string& analysisPass : settings.analysisPasses)
	{
		if (analysisPass == "pso-stats")
		{
			PsoStatsPassResult analysisResult;
			std::string analysisErrorMessage;
			if (!PsoStatsPass::WriteCsv(
			        cookResult.packages,
			        cookResult.cacheDirectory / "Analysis",
			        analysisResult,
			        analysisErrorMessage))
			{
				ToolConsole::Message(
				    std::cerr,
				    ToolConsoleSeverity::Error,
				    "Failed to run analysis pass",
				    {ToolConsole::QuotedField("analysis", "pso-stats"), ToolConsole::QuotedField("reason", analysisErrorMessage)});
				return kExitCodeCookFailure;
			}

			ToolConsole::Message(
			    std::cout,
			    ToolConsoleSeverity::Info,
			    "Analysis pass wrote output",
			    {ToolConsole::QuotedField("analysis", "pso-stats"),
			     ToolConsole::Field("rows", std::to_string(analysisResult.rowCount)),
			     ToolConsole::PathField("output", analysisResult.outputPath)});
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

