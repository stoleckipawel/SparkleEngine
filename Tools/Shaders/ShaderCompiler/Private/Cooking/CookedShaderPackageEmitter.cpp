#include "PCH.h"

#include "Cooking/CookedShaderPackageEmitter.h"

#include "Cooking/CookedPackageWriter.h"
#include "Cooking/CookedRegistryWriter.h"
#include "Cooking/ShaderCookResult.h"
#include "Cooking/ShaderRecookSignal.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <vector>

bool CookedShaderPackageEmitter::Emit(
    const ShaderCookPipelinePlan& plan,
    const std::filesystem::path& cacheDirectory,
    ShaderPackageCookResult& result,
    std::string& outErrorMessage)
{
	result.packages.clear();
	result.packages.reserve(plan.packages.size());
	std::vector<Files::FilePublication> publication;
	publication.reserve(plan.packages.size() + 1);
	for (std::size_t packageIndex = 0; packageIndex < plan.packages.size(); ++packageIndex)
	{
		const ShaderCookPackageDesc& package = plan.packages[packageIndex];
		const ShaderCookPackageContext& packageContext = plan.packageContexts[packageIndex];

		CookedShaderPackageOutput packageOutput;
		const std::filesystem::path publishedPath = Paths::CookedShaderPackage(BuildShaderPackageKey(package.packageId));
		const std::filesystem::path stagedPath = Files::BuildTemporaryPath(publishedPath, ".cook-generation");
		Files::CleanupTemporaryFile(stagedPath);
		if (!CookedPackageWriter::Write(package, packageContext.compiledStages, stagedPath, publishedPath, packageOutput, outErrorMessage))
		{
			outErrorMessage = "Failed to emit cooked shader package '" + package.packageId + "' - " + outErrorMessage;
			result.packages.clear();
			return false;
		}

		publication.push_back({stagedPath, publishedPath});
		result.packages.push_back(std::move(packageOutput));
	}

	result.registryPath = Filesystem::GetCookedShaderRegistryPath();
	const std::filesystem::path stagedRegistryPath = Files::BuildTemporaryPath(result.registryPath, ".cook-generation");
	Files::CleanupTemporaryFile(stagedRegistryPath);
	if (!CookedRegistryWriter::Write(result.packages, stagedRegistryPath, outErrorMessage))
	{
		result.packages.clear();
		return false;
	}
	ShaderRecookSignalResult signalResult;
	const std::filesystem::path publishedSignalPath = Paths::ShaderRecookSignal(cacheDirectory);
	const std::filesystem::path stagedSignalPath = Files::BuildTemporaryPath(publishedSignalPath, ".cook-generation");
	Files::CleanupTemporaryFile(stagedSignalPath);
	if (!ShaderRecookSignal::Write(
	        stagedRegistryPath,
	        result.registryPath,
	        stagedSignalPath,
	        publishedSignalPath,
	        signalResult,
	        outErrorMessage))
	{
		result.packages.clear();
		return false;
	}
	publication.push_back({stagedRegistryPath, result.registryPath});
	publication.push_back({stagedSignalPath, publishedSignalPath});
	if (!Files::TryPublishFileSet(publication, outErrorMessage))
	{
		for (const Files::FilePublication& file : publication)
			Files::CleanupTemporaryFile(file.StagedPath);
		result.packages.clear();
		return false;
	}
	result.recookSignalPath = signalResult.signalPath;
	result.recookSignalRegistryHash = signalResult.registryHash;

	outErrorMessage.clear();
	return true;
}
