#include "PCH.h"

#include "Cli/CookShadersCommand.h"

#include "Analysis/PsoStatsPass.h"
#include "Backend/ShaderTarget.h"
#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/ShaderPackageCooker.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"

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
		std::cerr << "ShaderCompiler: invalid cook arguments - " << parseErrorMessage << "\n";
		return kExitCodeUsage;
	}

	ShaderPackageCooker cooker;
	const ShaderPackageCookResult cookResult = cooker.CookAll(settings);
	if (!cookResult.Succeeded())
	{
		std::cerr << "ShaderCompiler: failed to cook shader packages - " << cookResult.errorMessage << "\n";
		return kExitCodeCookFailure;
	}

	std::cout << "ShaderCompiler: cooked " << cookResult.packages.size() << " shader package(s) under '"
	          << Paths::CookedShaderPackageRoot().string() << "'"
	          << " and registry '" << cookResult.registryPath.string() << "'"
	          << "; targets='" << FormatTargets(settings.targets) << "'"
	          << "; recookSignal='" << cookResult.recookSignalPath.string() << "'"
	          << "; backendInvocations=" << cookResult.backendInvocationCount
	          << ", cacheHits=" << cookResult.cacheHitCount
	          << ", cacheMisses=" << cookResult.cacheMissCount
	          << ", cacheDir='" << cookResult.cacheDirectory.string() << "'\n";

	for (const CookedShaderPackageOutput& package : cookResult.packages)
	{
		std::cout << "  Package '" << package.packageId << "' bindingLayout='"
		          << package.bindingLayoutId << "' key=" << Formatting::FormatHexUInt64(package.packageKey)
		          << " output='" << package.outputPath.string() << "'\n";
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
				std::cerr << "ShaderCompiler: failed to run analysis pass 'pso-stats' - " << analysisErrorMessage << "\n";
				return kExitCodeCookFailure;
			}

			std::cout << "ShaderCompiler: analysis 'pso-stats' wrote " << analysisResult.rowCount
			          << " row(s) to '" << analysisResult.outputPath.string() << "'\n";
			continue;
		}

		std::cerr << "ShaderCompiler: unknown analysis pass '" << analysisPass << "'\n";
		return kExitCodeUsage;
	}

	return kExitCodeSuccess;
}

