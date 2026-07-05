#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Cli/CookShadersCommand.h"

#include "Analysis/CookedShaderStatsPass.h"
#include "Backend/ShaderTarget.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/ShaderPackageCooker.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"
#include "ToolConsole.h"

#include <iostream>
#include <ostream>
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

void CookShadersCommand::PrintHelp(std::ostream& output)
{
	output << "Usage:\n"
	       << "  ShaderCompiler cook [--package <package-id> | --shader-id <registered-shader-name>] [options]\n\n"
	       << "Cooks typed shader registrations into .sparkshader packages and ShaderPackageRegistry.sreg.\n\n"
	       << "Selection options:\n"
	       << "  --package <package-id>      Cook one registered shader package.\n"
	       << "  --shader-id <shader-id>     Cook one registered shader entry by shader id.\n\n"
	       << "Cook options:\n"
	       << "  --no-cache                  Disable the local compile cache.\n"
	       << "  --cache-dir <path>          Override the local compile cache directory.\n"
	       << "  --target <name>             Add a codegen target such as DxilSm66 or SpirV16. "
	          "Defaults to DxilSm66 and SpirV16.\n"
	       << "  --backend <name>            Select a compiler backend, or auto.\n"
	       << "  --debug-artifacts <dir>     Write debug artifact bundles outside runtime packages.\n"
	       << "  --analysis <pass[,pass]>    Run optional analysis report passes such as cooked-shader-stats.\n"
	       << "  --debug-info                Request backend debug information and symbol emission where supported.\n"
	       << "  --disable-optimizations     Build shaders without backend optimization passes.\n"
	       << "  --warnings-as-errors <on|off>  Treat shader warnings as hard errors. Defaults to on.\n"
	       << "  --strip-reflection <on|off> Strip reflection data from final runtime binaries when supported.\n"
	       << "  --strip-debug <on|off>      Strip embedded debug info from final runtime binaries when supported.\n";
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

		if (args[index] == "--package")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --package";
				return false;
			}

			outSettings.packageId = std::string(args[index + 1]);
			++index;
			continue;
		}

		if (args[index] == "--shader-id")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --shader-id";
				return false;
			}

			outSettings.shaderId = std::string(args[index + 1]);
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

		if (args[index] == "--debug-info")
		{
			outSettings.enableDebugInfo = true;
			continue;
		}

		if (args[index] == "--disable-optimizations")
		{
			outSettings.enableOptimizations = false;
			continue;
		}

		if (args[index] == "--warnings-as-errors")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --warnings-as-errors";
				return false;
			}
			const std::string_view value = args[index + 1];
			if (value == "on" || value == "true")
			{
				outSettings.treatWarningsAsErrors = true;
			}
			else if (value == "off" || value == "false")
			{
				outSettings.treatWarningsAsErrors = false;
			}
			else
			{
				outErrorMessage = "Expected on|off after --warnings-as-errors";
				return false;
			}
			++index;
			continue;
		}

		if (args[index] == "--strip-reflection")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --strip-reflection";
				return false;
			}
			const std::string_view value = args[index + 1];
			if (value == "on" || value == "true")
			{
				outSettings.stripReflection = true;
			}
			else if (value == "off" || value == "false")
			{
				outSettings.stripReflection = false;
			}
			else
			{
				outErrorMessage = "Expected on|off after --strip-reflection";
				return false;
			}
			++index;
			continue;
		}

		if (args[index] == "--strip-debug")
		{
			if (index + 1 >= args.size())
			{
				outErrorMessage = "Missing value after --strip-debug";
				return false;
			}
			const std::string_view value = args[index + 1];
			if (value == "on" || value == "true")
			{
				outSettings.stripDebugInfo = true;
			}
			else if (value == "off" || value == "false")
			{
				outSettings.stripDebugInfo = false;
			}
			else
			{
				outErrorMessage = "Expected on|off after --strip-debug";
				return false;
			}
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

	if (!outSettings.packageId.empty() && !outSettings.shaderId.empty())
	{
		outErrorMessage = "Use either --package or --shader-id, not both";
		return false;
	}

	outErrorMessage.clear();
	return true;
}

int CookShadersCommand::Run(std::span<const std::string_view> args) const
{
	if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h"))
	{
		PrintHelp(std::cout);
		return kExitCodeSuccess;
	}

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
	    "Shader cook summary",
	    {ToolConsole::Field("packages", std::to_string(cookResult.packages.size())),
	     ToolConsole::PathField("packageRoot", Filesystem::GetCookedShaderPackageRootPath()),
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
		if (analysisPass == "cooked-shader-stats")
		{
			CookedShaderStatsPassResult analysisResult;
			std::string analysisErrorMessage;
			if (!CookedShaderStatsPass::WriteCsv(
			        cookResult.packages,
			        cookResult.cacheDirectory / "Analysis",
			        analysisResult,
			        analysisErrorMessage))
			{
				ToolConsole::Message(
				    std::cerr,
				    ToolConsoleSeverity::Error,
				    "Failed to run analysis pass",
				    {ToolConsole::QuotedField("analysis", "cooked-shader-stats"), ToolConsole::QuotedField("reason", analysisErrorMessage)});
				return kExitCodeCookFailure;
			}

			ToolConsole::Message(
			    std::cout,
			    ToolConsoleSeverity::Info,
			    "Analysis pass wrote output",
			    {ToolConsole::QuotedField("analysis", "cooked-shader-stats"),
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

