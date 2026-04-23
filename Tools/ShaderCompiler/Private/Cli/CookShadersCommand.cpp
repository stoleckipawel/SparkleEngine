#include "PCH.h"

#include "Cli/CookShadersCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/ShaderPackageCooker.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <iostream>

bool CookShadersCommand::TryParseArguments(
	std::span<const std::string_view> args,
	ShaderPackageCookSettings& outSettings,
	std::string& outErrorMessage)
{
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
	          << ::GetCookedShaderPackageRootPath().string() << "'"
	          << " and registry '" << cookResult.registryPath.string() << "'"
	          << "; backendInvocations=" << cookResult.backendInvocationCount
	          << ", cacheHits=" << cookResult.cacheHitCount
	          << ", cacheMisses=" << cookResult.cacheMissCount
	          << ", cacheDir='" << cookResult.cacheDirectory.string() << "'\n";

	for (const CookedShaderPackageOutput& package : cookResult.packages)
	{
		std::cout << "  Package '" << package.packageId << "' variant='" << package.variantId << "' bindingLayout='"
		          << package.bindingLayoutId << "' key=" << std::hex << package.packageKey << std::dec
		          << " output='" << package.outputPath.string() << "'\n";
	}

	return kExitCodeSuccess;
}

