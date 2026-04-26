#include "PCH.h"

#include "Cooking/CookedShaderPackageEmitter.h"

#include "Cooking/CookedPackageWriter.h"
#include "Cooking/CookedRegistryWriter.h"
#include "Cooking/ShaderPackageCooker.h"
#include "Cooking/ShaderRecookSignal.h"

bool CookedShaderPackageEmitter::Emit(
    const ShaderCookPipelinePlan& plan,
    const std::filesystem::path& cacheDirectory,
    ShaderPackageCookResult& result,
    std::string& outErrorMessage)
{
	result.packages.clear();
	result.packages.reserve(plan.packages.size());
	for (std::size_t packageIndex = 0; packageIndex < plan.packages.size(); ++packageIndex)
	{
		const ShaderCookPackageDesc& package = plan.packages[packageIndex];
		const ShaderCookPackageContext& packageContext = plan.packageContexts[packageIndex];

		CookedShaderPackageOutput packageOutput;
		if (!CookedPackageWriter::Write(package, packageContext.compiledStages, packageOutput, outErrorMessage))
		{
			outErrorMessage = "Failed to emit cooked shader package '" + package.packageId + "' - " + outErrorMessage;
			result.packages.clear();
			return false;
		}

		result.packages.push_back(std::move(packageOutput));
	}

	if (!CookedRegistryWriter::Write(result.packages, result.registryPath, outErrorMessage))
	{
		result.packages.clear();
		return false;
	}

	ShaderRecookSignalResult signalResult;
	if (!ShaderRecookSignal::Write(cacheDirectory, result.registryPath, signalResult, outErrorMessage))
	{
		result.packages.clear();
		return false;
	}
	result.recookSignalPath = signalResult.signalPath;
	result.recookSignalRegistryHash = signalResult.registryHash;

	outErrorMessage.clear();
	return true;
}