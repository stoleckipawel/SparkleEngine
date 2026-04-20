#include "PCH.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Cooking/CookedPackageWriter.h"
#include "Cooking/CookedRegistryWriter.h"
#include "Cooking/CookedStageBuild.h"
#include "Cooking/StageCompiler.h"
#include "Manifest/ShaderCookManifest.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "RHI/Public/Shaders/ShaderPackageLayoutCatalog.h"

#include <format>

ShaderPackageCookResult ShaderPackageCooker::CookAll() const
{
	ShaderPackageCookResult result;
	ShaderCookManifest manifest;
	if (!manifest.LoadMerged(result.errorMessage))
	{
		return result;
	}

	result.packages.reserve(manifest.GetPackages().size());
	for (const ShaderCookPackageDesc& package : manifest.GetPackages())
	{
		PassParameterLayout bindingLayout;
		if (!ShaderPackageLayouts::TryBuild(package.bindingLayoutId, bindingLayout, result.errorMessage))
		{
			result.errorMessage =
			    "Failed to build binding layout for shader package '" + package.packageId + "' - " + result.errorMessage;
			result.packages.clear();
			return result;
		}

		std::vector<CookedStageBuild> compiledStages;
		compiledStages.reserve(package.stages.size());
		for (const ShaderCookStageDesc& stage : package.stages)
		{
			CookedStageBuild compiledStage;
			if (!StageCompiler::Compile(stage, compiledStage, result.errorMessage))
			{
				result.errorMessage = std::format(
				    "Failed to compile shader package '{}' variant '{}' stage '{}' - {}",
				    package.packageId,
				    package.variantId,
				    GetShaderStagePrefix(stage.stage),
				    result.errorMessage);
				result.packages.clear();
				return result;
			}
			compiledStages.push_back(std::move(compiledStage));
		}

		CookedShaderPackageOutput packageOutput;
		if (!CookedPackageWriter::Write(package, bindingLayout, compiledStages, packageOutput, result.errorMessage))
		{
			result.errorMessage =
			    "Failed to emit cooked shader package '" + package.packageId + "' - " + result.errorMessage;
			result.packages.clear();
			return result;
		}

		result.packages.push_back(std::move(packageOutput));
	}

	if (!CookedRegistryWriter::Write(result.packages, result.registryPath, result.errorMessage))
	{
		result.packages.clear();
		return result;
	}

	return result;
}

